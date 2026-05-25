#ifndef RENDERER_H
#define RENDERER_H

#include "camera.h"
#include "hittable.h"
#include "image.h"
#include "interval.h"
#include "material.h"
#include "render_config.h"
#include "render_progress.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct render_scene {
    std::string name;
    camera cam;
    std::shared_ptr<hittable> world;
};

struct render_result {
    image framebuffer;
};

namespace renderer_detail {

struct prepared_render {
    camera cam;
    std::shared_ptr<const antialiasing> sampling_strategy;
    double sample_scale = 1.0;
    int sample_count = 1;
};

struct render_tile {
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
};

enum class random_stream : std::uint64_t {
    sample_pattern = 1,
    camera = 2,
    path_scatter = 3,
};

inline std::uint64_t sample_stream_seed(
    std::uint64_t base_seed,
    std::uint64_t pixel_index,
    int sample_index,
    random_stream stream
) {
    auto seed = combine_seed(base_seed, pixel_index);
    seed = combine_seed(seed, static_cast<std::uint64_t>(sample_index));
    return combine_seed(seed, static_cast<std::uint64_t>(stream));
}

inline prepared_render prepare_render(const render_scene& scene, const render_config& config) {
    prepared_render prepared;
    prepared.cam = scene.cam;
    prepared.cam.prepare(config.image_width, config.aspect_ratio);
    prepared.sampling_strategy = config.sampling_strategy
        ? config.sampling_strategy
        : std::make_shared<antialiasing_off>();
    prepared.sample_scale = prepared.sampling_strategy->sample_scale();
    prepared.sample_count = prepared.sampling_strategy->sample_count();
    return prepared;
}

inline color ray_color(
    const ray& r,
    int depth,
    const hittable& world,
    const color& background,
    random_source& rng,
    const hit_context& context
) {
    if (depth <= 0) {
        return color(0, 0, 0);
    }

    hit_record rec;
    if (!world.hit(r, interval(0.001, infinity), rec, context)) {
        return background;
    }

    ray scattered;
    color attenuation;
    color color_from_emission = rec.mat->emitted(rec.u, rec.v, rec.p);

    if (!rec.mat->scatter(r, rec, attenuation, scattered, rng)) {
        return color_from_emission;
    }

    auto next_context = context;
    ++next_context.depth;
    color color_from_scatter = attenuation * ray_color(
        scattered,
        depth - 1,
        world,
        background,
        rng,
        next_context
    );
    return color_from_emission + color_from_scatter;
}

inline color render_pixel(
    int i,
    int j,
    int image_width,
    const prepared_render& prepared,
    const render_scene& scene,
    const render_config& config
) {
    const auto pixel_index = static_cast<std::uint64_t>(j) * image_width + i;
    color pixel_color(0, 0, 0);

    for (int sample_index = 0; sample_index < prepared.sample_count; ++sample_index) {
        random_source sample_pattern_rng(sample_stream_seed(
            config.sampling_seed,
            pixel_index,
            sample_index,
            random_stream::sample_pattern
        ));
        random_source camera_rng(sample_stream_seed(
            config.sampling_seed,
            pixel_index,
            sample_index,
            random_stream::camera
        ));
        random_source path_rng(sample_stream_seed(
            config.sampling_seed,
            pixel_index,
            sample_index,
            random_stream::path_scatter
        ));

        const auto sample = prepared.sampling_strategy->sample(sample_index, sample_pattern_rng);
        ray r = prepared.cam.get_ray(i, j, sample, camera_rng);
        hit_context context{config.sampling_seed, pixel_index, sample_index, 0};
        pixel_color += ray_color(
            r,
            config.max_depth,
            *scene.world,
            config.background,
            path_rng,
            context
        ) * sample.weight;
    }

    return pixel_color * prepared.sample_scale;
}

inline std::vector<render_tile> make_tiles(int width, int height, int tile_size) {
    const int size = std::max(1, tile_size);
    std::vector<render_tile> tiles;

    for (int y = 0; y < height; y += size) {
        for (int x = 0; x < width; x += size) {
            tiles.push_back({
                x,
                y,
                std::min(x + size, width),
                std::min(y + size, height)
            });
        }
    }

    return tiles;
}

inline int actual_worker_count(int requested_thread_count, size_t tile_count) {
    return std::max(
        1,
        std::min(
            std::max(1, requested_thread_count),
            static_cast<int>(std::max<size_t>(1, tile_count))
        )
    );
}

inline int actual_worker_count(int width, int height, int tile_size, int requested_thread_count) {
    return actual_worker_count(
        requested_thread_count,
        make_tiles(width, height, tile_size).size()
    );
}

} // namespace renderer_detail

class renderer {
public:
    virtual ~renderer() = default;
    virtual render_result render(const render_scene& scene, const render_config& config) const = 0;
};

class single_thread_renderer final : public renderer {
public:
    render_result render(const render_scene& scene, const render_config& config) const override {
        const auto prepared = renderer_detail::prepare_render(scene, config);
        image framebuffer(prepared.cam.prepared_image_width(), prepared.cam.prepared_image_height());

        std::unique_ptr<render_progress> progress;
        if (config.show_progress) {
            progress = std::make_unique<render_progress>(framebuffer.height());
            progress->update(0, true);
        }

        for (int j = 0; j < framebuffer.height(); ++j) {
            for (int i = 0; i < framebuffer.width(); ++i) {
                framebuffer.pixel(i, j) = renderer_detail::render_pixel(
                    i,
                    j,
                    framebuffer.width(),
                    prepared,
                    scene,
                    config
                );
            }

            if (config.show_progress) {
                progress->update(j + 1, false);
            }
        }

        if (config.show_progress) {
            progress->done();
        }

        return {std::move(framebuffer)};
    }
};

class multi_thread_renderer final : public renderer {
public:
    render_result render(const render_scene& scene, const render_config& config) const override {
        const auto prepared = renderer_detail::prepare_render(scene, config);
        image framebuffer(prepared.cam.prepared_image_width(), prepared.cam.prepared_image_height());
        const auto tiles = renderer_detail::make_tiles(
            framebuffer.width(),
            framebuffer.height(),
            config.tile_size
        );

        const int worker_count = renderer_detail::actual_worker_count(config.thread_count, tiles.size());

        std::unique_ptr<render_progress> progress;
        std::mutex progress_mutex;
        std::atomic<int> completed_tiles{0};
        if (config.show_progress) {
            progress = std::make_unique<render_progress>(static_cast<int>(tiles.size()));
            progress->update(0, true);
        }

        auto render_worker = [&](int worker_index) {
            for (size_t tile_index = static_cast<size_t>(worker_index);
                 tile_index < tiles.size();
                 tile_index += static_cast<size_t>(worker_count)) {
                const auto& tile = tiles[tile_index];
                for (int y = tile.y0; y < tile.y1; ++y) {
                    for (int x = tile.x0; x < tile.x1; ++x) {
                        framebuffer.pixel(x, y) = renderer_detail::render_pixel(
                            x,
                            y,
                            framebuffer.width(),
                            prepared,
                            scene,
                            config
                        );
                    }
                }

                if (progress) {
                    std::lock_guard<std::mutex> lock(progress_mutex);
                    const int done = ++completed_tiles;
                    progress->update(done, false);
                }
            }
        };

        std::vector<std::thread> workers;
        workers.reserve(static_cast<size_t>(worker_count));
        for (int worker_index = 0; worker_index < worker_count; ++worker_index) {
            workers.emplace_back(render_worker, worker_index);
        }

        for (auto& worker : workers) {
            worker.join();
        }

        if (config.show_progress) {
            progress->done();
        }

        return {std::move(framebuffer)};
    }
};

#endif
