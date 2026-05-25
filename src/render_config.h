#ifndef RENDER_CONFIG_H
#define RENDER_CONFIG_H

#include "acceleration.h"
#include "antialiasing.h"
#include "color.h"

#include <cstdint>
#include <memory>
#include <optional>

struct render_config {
    double aspect_ratio = 16.0 / 9.0;
    int image_width = 800;
    int max_depth = 50;
    std::uint64_t sampling_seed = 1;
    color background = color(0, 0, 0);
    bool show_progress = true;
    int thread_count = 1;
    int tile_size = 16;
    std::shared_ptr<const antialiasing> sampling_strategy = std::make_shared<antialiasing_off>();
};

struct scene_settings {
    double aspect_ratio = 16.0 / 9.0;
    int image_width = 800;
    int max_depth = 50;
    color background = color(0, 0, 0);
    acceleration_structure default_acceleration = acceleration_structure::plain;
    std::shared_ptr<const antialiasing> default_sampling_strategy = std::make_shared<antialiasing_off>();
};

struct experiment_settings {
    std::uint64_t sampling_seed = 1;
    std::uint64_t build_seed = 1;
    std::optional<acceleration_structure> acceleration;
    bool show_progress = true;
    int thread_count = 1;
    int tile_size = 16;
    std::shared_ptr<const antialiasing> sampling_strategy;
};

inline render_config make_render_config(
    const scene_settings& scene,
    const experiment_settings& experiment
) {
    render_config config;
    config.aspect_ratio = scene.aspect_ratio;
    config.image_width = scene.image_width;
    config.max_depth = scene.max_depth;
    config.background = scene.background;
    config.sampling_seed = experiment.sampling_seed;
    config.show_progress = experiment.show_progress;
    config.thread_count = experiment.thread_count;
    config.tile_size = experiment.tile_size;
    config.sampling_strategy = experiment.sampling_strategy
        ? experiment.sampling_strategy
        : scene.default_sampling_strategy;
    return config;
}

#endif
