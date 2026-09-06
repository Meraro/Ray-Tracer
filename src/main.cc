#include "benchmark.h"
#include "renderer.h"
#include "scene_presets.h"
#include "world_builder.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

struct sampling_override {
    bool enabled = false;
    sampling_pattern pattern = sampling_pattern::aa_off;
    int parameter = 1;
};

struct parsed_arguments {
    std::vector<std::string> positional;
    std::string reference_path;
    std::string output_path;
    std::string error;
    bool show_progress = true;
    bool benchmark = false;
    bool disable_internal_bvh = false;
    int thread_count = 1;
    int tile_size = 16;
    std::optional<int> image_width;
};

bool parse_positive_int_value(const std::string& value, int& parsed);

bool is_option_token(const std::string& value) {
    return value.rfind("--", 0) == 0;
}

parsed_arguments parse_arguments(int argc, char* argv[]) {
    parsed_arguments result;

    for (int index = 1; index < argc; ++index) {
        const std::string value = argv[index];
        if (value == "--benchmark") {
            result.benchmark = true;
            continue;
        }
        if (value == "--disable-internal-bvh") {
            result.disable_internal_bvh = true;
            continue;
        }
        if (value == "--width") {
            int width = 0;
            if (index + 1 >= argc || !parse_positive_int_value(argv[++index], width)) {
                result.error = "invalid_image_width";
            } else {
                result.image_width = width;
            }
            continue;
        }
        if (value == "--reference" || value == "--ref") {
            if (index + 1 < argc && !is_option_token(argv[index + 1])) {
                result.reference_path = argv[++index];
            } else {
                result.error = "missing_reference_path";
            }
            continue;
        }

        if (value == "--output" || value == "--out") {
            if (index + 1 < argc && !is_option_token(argv[index + 1])) {
                result.output_path = argv[++index];
            } else {
                result.error = "missing_output_path";
            }
            continue;
        }

        if (value == "--no-progress") {
            result.show_progress = false;
            continue;
        }

        if (value == "--progress") {
            result.show_progress = true;
            continue;
        }

        if (value == "--threads") {
            if (index + 1 < argc) {
                const std::string thread_count = argv[++index];
                char* end = nullptr;
                errno = 0;
                const long parsed_thread_count = std::strtol(thread_count.c_str(), &end, 10);
                if (errno == ERANGE || end == thread_count.c_str() || *end != '\0' || parsed_thread_count <= 0) {
                    result.error = "invalid_thread_count";
                } else {
                    result.thread_count = static_cast<int>(parsed_thread_count);
                }
            } else {
                result.error = "missing_thread_count";
            }
            continue;
        }

        if (value == "--tile-size") {
            if (index + 1 < argc) {
                const std::string tile_size = argv[++index];
                char* end = nullptr;
                errno = 0;
                const long parsed_tile_size = std::strtol(tile_size.c_str(), &end, 10);
                if (errno == ERANGE || end == tile_size.c_str() || *end != '\0' || parsed_tile_size <= 0) {
                    result.error = "invalid_tile_size";
                } else {
                    result.tile_size = static_cast<int>(parsed_tile_size);
                }
            } else {
                result.error = "missing_tile_size";
            }
            continue;
        }

        if (is_option_token(value)) {
            result.error = "unknown_option";
            continue;
        }

        result.positional.push_back(value);
    }

    return result;
}

bool parse_uint64_value(const std::string& value, std::uint64_t& parsed) {
    if (value.empty() || value[0] == '-') {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    const auto raw = std::strtoull(value.c_str(), &end, 10);
    if (errno == ERANGE || end == value.c_str() || *end != '\0') {
        return false;
    }

    parsed = static_cast<std::uint64_t>(raw);
    return true;
}

bool parse_positive_int_value(const std::string& value, int& parsed) {
    std::uint64_t raw = 0;
    if (!parse_uint64_value(value, raw) || raw == 0 || raw > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
        return false;
    }

    parsed = static_cast<int>(raw);
    return true;
}

scene_id parse_scene_id(const std::vector<std::string>& positional, std::string& error) {
    if (positional.empty()) {
        return scene_id::cornell_smoke;
    }

    int raw_scene_id = 0;
    if (!parse_positive_int_value(positional[0], raw_scene_id)) {
        error = "invalid_scene_id";
        return scene_id::cornell_smoke;
    }

    switch (raw_scene_id) {
        case 1: return scene_id::bouncing_spheres;
        case 2: return scene_id::checkered_spheres;
        case 3: return scene_id::earth;
        case 4: return scene_id::perlin_spheres;
        case 5: return scene_id::quads;
        case 6: return scene_id::simple_light;
        case 7: return scene_id::cornell_box;
        case 8: return scene_id::cornell_smoke;
        case 9: return scene_id::final_scene;
        default:
            error = "invalid_scene_id";
            return scene_id::cornell_smoke;
    }
}

std::uint64_t parse_seed_arg(
    const std::vector<std::string>& positional,
    size_t index,
    std::uint64_t fallback,
    const char* error_name,
    std::string& error
) {
    if (index >= positional.size()) {
        return fallback;
    }

    std::uint64_t parsed = 0;
    if (!parse_uint64_value(positional[index], parsed)) {
        error = error_name;
        return fallback;
    }

    return parsed;
}

int parse_int_arg(
    const std::vector<std::string>& positional,
    size_t index,
    int fallback,
    const char* error_name,
    std::string& error
) {
    if (index >= positional.size()) {
        return fallback;
    }

    int parsed = 0;
    if (!parse_positive_int_value(positional[index], parsed)) {
        error = error_name;
        return fallback;
    }

    return parsed;
}

bool parse_sampling_pattern_value(const std::string& value, sampling_pattern& pattern) {
    if (value == "off" || value == "aa_off" || value == "none" || value == "0") {
        pattern = sampling_pattern::aa_off;
        return true;
    }

    if (value == "random") {
        pattern = sampling_pattern::random;
        return true;
    }

    if (value == "grid") {
        pattern = sampling_pattern::grid;
        return true;
    }

    if (value == "jittered" || value == "jittered_grid") {
        pattern = sampling_pattern::jittered_grid;
        return true;
    }

    return false;
}

sampling_override parse_sampling_override(
    const std::vector<std::string>& positional,
    size_t pattern_index,
    size_t parameter_index,
    std::string& error
) {
    sampling_override result;
    if (pattern_index >= positional.size()) {
        return result;
    }

    sampling_pattern pattern;
    if (!parse_sampling_pattern_value(positional[pattern_index], pattern)) {
        error = "invalid_sampling_pattern";
        return result;
    }

    result.enabled = true;
    result.pattern = pattern;
    result.parameter = parse_int_arg(positional, parameter_index, 1, "invalid_sample_count", error);
    return result;
}

void write_quality_result(std::ostream& out, const quality_result& quality) {
    if (quality.available) {
        out << "quality,display_rgb_8bit_ppm,ok,"
            << std::scientific << std::setprecision(8) << quality.mse << ','
            << std::fixed << std::setprecision(6) << quality.psnr_db << '\n';
    } else {
        out << "quality,none," << (quality.error.empty() ? "none" : quality.error) << ",,\n";
    }
}

std::optional<acceleration_structure> parse_acceleration_arg(
    const std::vector<std::string>& positional,
    size_t index,
    std::string& error
) {
    if (index >= positional.size()) {
        return std::nullopt;
    }

    const std::string& value = positional[index];
    if (value == "plain" || value == "0") {
        return acceleration_structure::plain;
    }

    if (value == "bvh" || value == "1") {
        return acceleration_structure::bvh;
    }

    error = "invalid_acceleration";
    return std::nullopt;
}

bool requires_square_sample_count(sampling_pattern pattern) {
    return pattern == sampling_pattern::grid || pattern == sampling_pattern::jittered_grid;
}

} // namespace

int main(int argc, char* argv[]) {
    const auto args = parse_arguments(argc, argv);
    if (!args.error.empty()) {
        std::cerr << "error," << args.error << '\n';
        return 1;
    }

    std::string parse_error;
    const auto selected_scene = parse_scene_id(args.positional, parse_error);
    const auto scene_seed = parse_seed_arg(args.positional, 1, 1, "invalid_scene_seed", parse_error);
    const auto sampling_seed = parse_seed_arg(args.positional, 2, 1, "invalid_sampling_seed", parse_error);
    const auto acceleration_override = parse_acceleration_arg(args.positional, 3, parse_error);
    const auto build_seed = parse_seed_arg(
        args.positional,
        4,
        combine_seed(scene_seed, 0xb71d5eedULL),
        "invalid_build_seed",
        parse_error
    );
    const auto& reference_path = args.reference_path;
    const auto& output_path = args.output_path;
    sampling_pattern maybe_pattern;
    const bool sampling_starts_at_run_arg =
        args.positional.size() > 5 && parse_sampling_pattern_value(args.positional[5], maybe_pattern);
    const auto runs = sampling_starts_at_run_arg
        ? 1
        : parse_int_arg(args.positional, 5, 1, "invalid_runs", parse_error);
    const auto sampling = sampling_starts_at_run_arg
        ? parse_sampling_override(args.positional, 5, 6, parse_error)
        : parse_sampling_override(args.positional, 6, 7, parse_error);

    if (!parse_error.empty()) {
        std::cerr << "error," << parse_error << '\n';
        return 1;
    }

    if (sampling.enabled &&
        requires_square_sample_count(sampling.pattern) &&
        !is_square_sample_count(sampling.parameter)) {
        std::cerr << "error,grid_sampling_requires_perfect_square_sample_count,"
                  << sampling.parameter << '\n';
        return 1;
    }

    if (args.benchmark || runs > 1) {
        benchmark_case bench;
        bench.scene = selected_scene;
        bench.scene_seed = scene_seed;
        bench.use_internal_bvh = !args.disable_internal_bvh;
        bench.experiment.sampling_seed = sampling_seed;
        bench.experiment.build_seed = build_seed;
        bench.experiment.acceleration = acceleration_override;
        bench.experiment.thread_count = args.thread_count;
        bench.experiment.tile_size = args.tile_size;
        bench.experiment.image_width = args.image_width;
        if (sampling.enabled) {
            bench.experiment.sampling_strategy = make_sampling_strategy(sampling.pattern, sampling.parameter);
            bench.sampling_name = sampling_pattern_name(sampling.pattern);
            bench.requested_sample_count = sampling.pattern == sampling_pattern::aa_off
                ? 1
                : sampling.parameter;
        }
        bench.reference_path = reference_path;
        bench.output_path = output_path;
        bench.runs = runs;

        const auto results = run_benchmark(bench);
        write_benchmark_results(std::cerr, results);
        return 0;
    }

    auto preset = make_scene(selected_scene, scene_seed, !args.disable_internal_bvh);
    experiment_settings experiment;
    experiment.sampling_seed = sampling_seed;
    experiment.acceleration = acceleration_override;
    experiment.build_seed = build_seed;
    experiment.show_progress = args.show_progress;
    experiment.thread_count = args.thread_count;
    experiment.tile_size = args.tile_size;
    experiment.image_width = args.image_width;
    if (sampling.enabled) {
        experiment.sampling_strategy = make_sampling_strategy(sampling.pattern, sampling.parameter);
    }
    const auto config = make_render_config(preset.settings, experiment);

    const auto acceleration = experiment.acceleration.value_or(preset.settings.default_acceleration);
    auto world = build_world(preset.geometry, acceleration, experiment.build_seed);
    render_scene scene{preset.name, preset.cam, std::move(world)};

    const auto prepared_height = static_cast<int>(config.image_width / config.aspect_ratio) < 1
        ? 1
        : static_cast<int>(config.image_width / config.aspect_ratio);
    const auto actual_worker_count = renderer_detail::actual_worker_count(
        config.image_width,
        prepared_height,
        config.tile_size,
        config.thread_count
    );
    const auto use_multi_thread = actual_worker_count > 1;
    single_thread_renderer single_renderer;
    multi_thread_renderer multi_renderer;
    const auto result = use_multi_thread
        ? multi_renderer.render(scene, config)
        : single_renderer.render(scene, config);
    if (!reference_path.empty()) {
        const auto reference = load_ppm_reference(reference_path);
        const auto quality = reference.pixels
            ? compare_to_reference(result.framebuffer, *reference.pixels)
            : quality_result{false, 0.0, 0.0, reference.error};
        write_quality_result(std::cerr, quality);
    }
    if (!output_path.empty()) {
        std::ofstream output(output_path, std::ios::binary);
        if (!output) {
            std::cerr << "error,could_not_open_output," << output_path << '\n';
            return 1;
        }
        result.framebuffer.write_ppm(output);
    } else {
        result.framebuffer.write_ppm(std::cout);
    }
}
