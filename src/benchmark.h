#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "acceleration.h"
#include "image.h"
#include "quality_metrics.h"
#include "renderer.h"
#include "scene_presets.h"
#include "world_builder.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ios>
#include <limits>
#include <optional>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>

struct benchmark_case {
    scene_id scene;
    std::uint64_t scene_seed = 1;
    bool use_internal_bvh = true;
    experiment_settings experiment;
    std::string sampling_name = "scene_default";
    int requested_sample_count = 0;
    std::string reference_path;
    std::string output_path;
    int runs = 1;
};

struct benchmark_timing {
    double scene_build_ms = 0.0;
    double acceleration_build_ms = 0.0;
    double render_ms = 0.0;
    double serialization_ms = 0.0;
    double output_write_ms = 0.0;
    double serialization_component_total_ms = 0.0;
    double output_component_total_ms = 0.0;
};

struct benchmark_run_result {
    int run_index = 0;
    std::string scene_name;
    std::string backend;
    acceleration_structure acceleration = acceleration_structure::plain;
    std::string sampling_name;
    int requested_sample_count = 1;
    int actual_sample_count = 1;
    int requested_thread_count = 1;
    int actual_worker_count = 1;
    int tile_size = 16;
    quality_result quality;
    std::string output_write_status = "none";
    benchmark_timing timing;
};

struct benchmark_summary {
    benchmark_timing min;
    benchmark_timing max;
    benchmark_timing avg;
    quality_result quality_min;
    quality_result quality_max;
    quality_result quality_avg;
};

namespace benchmark_detail {

using clock = std::chrono::steady_clock;

inline double elapsed_ms(clock::time_point start, clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

class null_buffer final : public std::streambuf {
protected:
    int overflow(int ch) override {
        return ch;
    }

    std::streamsize xsputn(const char*, std::streamsize count) override {
        return count;
    }
};

inline benchmark_timing timing_min(const benchmark_timing& lhs, const benchmark_timing& rhs) {
    return {
        std::min(lhs.scene_build_ms, rhs.scene_build_ms),
        std::min(lhs.acceleration_build_ms, rhs.acceleration_build_ms),
        std::min(lhs.render_ms, rhs.render_ms),
        std::min(lhs.serialization_ms, rhs.serialization_ms),
        std::min(lhs.output_write_ms, rhs.output_write_ms),
        std::min(lhs.serialization_component_total_ms, rhs.serialization_component_total_ms),
        std::min(lhs.output_component_total_ms, rhs.output_component_total_ms),
    };
}

inline benchmark_timing timing_max(const benchmark_timing& lhs, const benchmark_timing& rhs) {
    return {
        std::max(lhs.scene_build_ms, rhs.scene_build_ms),
        std::max(lhs.acceleration_build_ms, rhs.acceleration_build_ms),
        std::max(lhs.render_ms, rhs.render_ms),
        std::max(lhs.serialization_ms, rhs.serialization_ms),
        std::max(lhs.output_write_ms, rhs.output_write_ms),
        std::max(lhs.serialization_component_total_ms, rhs.serialization_component_total_ms),
        std::max(lhs.output_component_total_ms, rhs.output_component_total_ms),
    };
}

inline benchmark_timing timing_add(const benchmark_timing& lhs, const benchmark_timing& rhs) {
    return {
        lhs.scene_build_ms + rhs.scene_build_ms,
        lhs.acceleration_build_ms + rhs.acceleration_build_ms,
        lhs.render_ms + rhs.render_ms,
        lhs.serialization_ms + rhs.serialization_ms,
        lhs.output_write_ms + rhs.output_write_ms,
        lhs.serialization_component_total_ms + rhs.serialization_component_total_ms,
        lhs.output_component_total_ms + rhs.output_component_total_ms,
    };
}

inline benchmark_timing timing_divide(const benchmark_timing& timing, double divisor) {
    return {
        timing.scene_build_ms / divisor,
        timing.acceleration_build_ms / divisor,
        timing.render_ms / divisor,
        timing.serialization_ms / divisor,
        timing.output_write_ms / divisor,
        timing.serialization_component_total_ms / divisor,
        timing.output_component_total_ms / divisor,
    };
}

inline double serialization_component_total_ms(const benchmark_timing& timing) {
    return timing.scene_build_ms
        + timing.acceleration_build_ms
        + timing.render_ms
        + timing.serialization_ms;
}

inline double output_component_total_ms(const benchmark_timing& timing) {
    return timing.scene_build_ms
        + timing.acceleration_build_ms
        + timing.render_ms
        + timing.output_write_ms;
}

inline std::string output_path_for_run(const std::string& output_path, int run_index, int runs) {
    if (output_path.empty() || runs <= 1) {
        return output_path;
    }

    const auto extension_pos = output_path.find_last_of('.');
    const auto separator_pos = output_path.find_last_of("/\\");
    const bool has_extension = extension_pos != std::string::npos &&
        (separator_pos == std::string::npos || extension_pos > separator_pos);

    std::ostringstream numbered_path;
    if (has_extension) {
        numbered_path << output_path.substr(0, extension_pos)
            << "_run" << run_index
            << output_path.substr(extension_pos);
    } else {
        numbered_path << output_path << "_run" << run_index;
    }
    return numbered_path.str();
}

} // namespace benchmark_detail

inline benchmark_run_result run_benchmark_once(
    const benchmark_case& bench,
    int run_index,
    const std::string& output_path,
    const image* reference,
    const std::string& reference_error
) {
    benchmark_run_result result;
    result.run_index = run_index;
    result.requested_thread_count = std::max(1, bench.experiment.thread_count);
    result.tile_size = bench.experiment.tile_size;

    auto experiment = bench.experiment;
    const auto scene_start = benchmark_detail::clock::now();
    auto preset = make_scene(bench.scene, bench.scene_seed, bench.use_internal_bvh);
    const auto scene_end = benchmark_detail::clock::now();

    const auto acceleration = experiment.acceleration.value_or(preset.settings.default_acceleration);
    experiment.show_progress = false;
    const auto config = make_render_config(preset.settings, experiment);

    result.scene_name = preset.name;
    result.acceleration = acceleration;
    result.actual_worker_count = renderer_detail::actual_worker_count(
        config.image_width,
        static_cast<int>(config.image_width / config.aspect_ratio) < 1
            ? 1
            : static_cast<int>(config.image_width / config.aspect_ratio),
        config.tile_size,
        config.thread_count
    );
    result.backend = result.actual_worker_count > 1 ? "multi_thread" : "single_thread";
    result.sampling_name = bench.sampling_name;
    result.actual_sample_count = config.sampling_strategy
        ? config.sampling_strategy->sample_count()
        : 1;
    result.requested_sample_count = bench.requested_sample_count > 0
        ? bench.requested_sample_count
        : result.actual_sample_count;
    result.timing.scene_build_ms = benchmark_detail::elapsed_ms(scene_start, scene_end);

    const auto build_start = benchmark_detail::clock::now();
    auto world = build_world(preset.geometry, acceleration, experiment.build_seed);
    const auto build_end = benchmark_detail::clock::now();
    result.timing.acceleration_build_ms = benchmark_detail::elapsed_ms(build_start, build_end);

    render_scene scene{preset.name, preset.cam, std::move(world)};
    single_thread_renderer single_renderer;
    multi_thread_renderer multi_renderer;
    const renderer& active_renderer = result.actual_worker_count > 1
        ? static_cast<const renderer&>(multi_renderer)
        : static_cast<const renderer&>(single_renderer);

    const auto render_start = benchmark_detail::clock::now();
    const auto render_result = active_renderer.render(scene, config);
    const auto render_end = benchmark_detail::clock::now();
    result.timing.render_ms = benchmark_detail::elapsed_ms(render_start, render_end);
    if (reference) {
        result.quality = compare_to_reference(render_result.framebuffer, *reference);
    } else if (!reference_error.empty()) {
        result.quality = {false, 0.0, 0.0, reference_error};
    }

    benchmark_detail::null_buffer null_buffer;
    std::ostream null_output(&null_buffer);

    const auto serialization_start = benchmark_detail::clock::now();
    render_result.framebuffer.write_ppm(null_output);
    const auto serialization_end = benchmark_detail::clock::now();
    result.timing.serialization_ms = benchmark_detail::elapsed_ms(serialization_start, serialization_end);

    if (!output_path.empty()) {
        const auto output_write_start = benchmark_detail::clock::now();
        std::ofstream output(output_path, std::ios::binary);
        if (output) {
            render_result.framebuffer.write_ppm(output);
            output.flush();
            const bool write_ok = static_cast<bool>(output);
            output.close();
            result.output_write_status = write_ok && output
                ? "ok" : "failed_to_write_output";
        } else {
            result.output_write_status = "failed_to_open_output";
        }
        const auto output_write_end = benchmark_detail::clock::now();
        result.timing.output_write_ms = benchmark_detail::elapsed_ms(output_write_start, output_write_end);
    }

    result.timing.serialization_component_total_ms =
        benchmark_detail::serialization_component_total_ms(result.timing);
    if (!output_path.empty()) {
        result.timing.output_component_total_ms =
            benchmark_detail::output_component_total_ms(result.timing);
    }

    return result;
}

inline std::vector<benchmark_run_result> run_benchmark(const benchmark_case& bench) {
    std::vector<benchmark_run_result> results;
    results.reserve(static_cast<size_t>(std::max(1, bench.runs)));

    std::optional<image> reference;
    std::string reference_error;
    if (!bench.reference_path.empty()) {
        auto loaded_reference = load_ppm_reference(bench.reference_path);
        if (loaded_reference.pixels) {
            reference = std::move(loaded_reference.pixels);
        } else {
            reference_error = loaded_reference.error;
        }
    }

    for (int run_index = 1; run_index <= bench.runs; ++run_index) {
        results.push_back(run_benchmark_once(
            bench,
            run_index,
            benchmark_detail::output_path_for_run(bench.output_path, run_index, bench.runs),
            reference ? &*reference : nullptr,
            reference_error
        ));
    }

    return results;
}

inline benchmark_summary summarize_benchmark(const std::vector<benchmark_run_result>& results) {
    benchmark_summary summary;
    if (results.empty()) {
        return summary;
    }

    const benchmark_timing infinity_timing{
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
    };

    summary.min = infinity_timing;

    benchmark_timing total;
    int quality_count = 0;
    double total_mse = 0.0;
    double min_mse = std::numeric_limits<double>::infinity();
    double max_mse = 0.0;

    for (const auto& result : results) {
        summary.min = benchmark_detail::timing_min(summary.min, result.timing);
        summary.max = benchmark_detail::timing_max(summary.max, result.timing);
        total = benchmark_detail::timing_add(total, result.timing);

        if (result.quality.available) {
            ++quality_count;
            total_mse += result.quality.mse;
            min_mse = std::min(min_mse, result.quality.mse);
            max_mse = std::max(max_mse, result.quality.mse);
        }
    }

    summary.avg = benchmark_detail::timing_divide(total, static_cast<double>(results.size()));
    if (quality_count > 0) {
        const auto avg_mse = total_mse / quality_count;
        summary.quality_min = {true, min_mse, psnr_from_mse(min_mse), ""};
        summary.quality_max = {true, max_mse, psnr_from_mse(max_mse), ""};
        summary.quality_avg = {
            true,
            avg_mse,
            psnr_from_mse(avg_mse),
            ""
        };
    }
    return summary;
}

inline void write_benchmark_header(std::ostream& out) {
    out << "type,run,scene,backend,requested_thread_count,actual_worker_count,tile_size,acceleration,sampling,requested_sample_count,actual_sample_count,quality_space,quality_status,mse,psnr_db,scene_build_ms,acceleration_build_ms,render_ms,serialization_ms,output_write_status,output_write_ms,serialization_component_total_ms,output_component_total_ms\n";
}

inline void write_output_component_total(
    std::ostream& out,
    const std::string& output_write_status,
    const benchmark_timing& timing
) {
    if (output_write_status == "none") {
        return;
    }

    out << timing.output_component_total_ms;
}

inline void write_benchmark_row(
    std::ostream& out,
    const char* type,
    const std::string& run,
    const std::string& scene_name,
    const std::string& backend,
    int requested_thread_count,
    int actual_worker_count,
    int tile_size,
    acceleration_structure acceleration,
    const std::string& sampling_name,
    int requested_sample_count,
    int actual_sample_count,
    const quality_result& quality,
    const std::string& output_write_status,
    const benchmark_timing& timing
) {
    out << type << ','
        << run << ','
        << scene_name << ','
        << backend << ','
        << requested_thread_count << ','
        << actual_worker_count << ','
        << tile_size << ','
        << acceleration_name(acceleration) << ','
        << sampling_name << ','
        << requested_sample_count << ','
        << actual_sample_count << ','
        << (quality.available ? "display_rgb_8bit_ppm" : "none") << ','
        << (quality.available ? "ok" : (quality.error.empty() ? "none" : quality.error)) << ',';

    if (quality.available) {
        out << std::scientific << std::setprecision(8)
            << quality.mse << ','
            << std::fixed << std::setprecision(6)
            << quality.psnr_db << ',';
    } else {
        out << ",,";
    }

    out << std::fixed << std::setprecision(3)
        << timing.scene_build_ms << ','
        << timing.acceleration_build_ms << ','
        << timing.render_ms << ','
        << timing.serialization_ms << ','
        << output_write_status << ','
        << timing.output_write_ms << ','
        << timing.serialization_component_total_ms << ',';
    write_output_component_total(out, output_write_status, timing);
    out << '\n';
}

inline void write_benchmark_results(std::ostream& out, const std::vector<benchmark_run_result>& results) {
    write_benchmark_header(out);

    for (const auto& result : results) {
        write_benchmark_row(
            out,
            "run",
            std::to_string(result.run_index),
            result.scene_name,
            result.backend,
            result.requested_thread_count,
            result.actual_worker_count,
            result.tile_size,
            result.acceleration,
            result.sampling_name,
            result.requested_sample_count,
            result.actual_sample_count,
            result.quality,
            result.output_write_status,
            result.timing
        );
    }

    if (results.empty()) {
        return;
    }

    const auto summary = summarize_benchmark(results);
    const auto& first = results.front();
    write_benchmark_row(out, "summary_min", "", first.scene_name, first.backend, first.requested_thread_count, first.actual_worker_count, first.tile_size, first.acceleration, first.sampling_name, first.requested_sample_count, first.actual_sample_count, summary.quality_min, first.output_write_status, summary.min);
    write_benchmark_row(out, "summary_avg", "", first.scene_name, first.backend, first.requested_thread_count, first.actual_worker_count, first.tile_size, first.acceleration, first.sampling_name, first.requested_sample_count, first.actual_sample_count, summary.quality_avg, first.output_write_status, summary.avg);
    write_benchmark_row(out, "summary_max", "", first.scene_name, first.backend, first.requested_thread_count, first.actual_worker_count, first.tile_size, first.acceleration, first.sampling_name, first.requested_sample_count, first.actual_sample_count, summary.quality_max, first.output_write_status, summary.max);
}

#endif
