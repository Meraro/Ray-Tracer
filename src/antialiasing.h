#ifndef ANTIALIASING_H
#define ANTIALIASING_H

#include "random.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>

enum class sampling_pattern {
    aa_off,
    random,
    grid,
    jittered_grid,
};

inline const char* sampling_pattern_name(sampling_pattern pattern) {
    switch (pattern) {
        case sampling_pattern::aa_off: return "aa_off";
        case sampling_pattern::random: return "random";
        case sampling_pattern::grid: return "grid";
        case sampling_pattern::jittered_grid: return "jittered_grid";
        default: return "unknown";
    }
}

struct aa_sample {
    double offset_u = 0.0;
    double offset_v = 0.0;
    double weight = 1.0;
};

class antialiasing {
    public:
        virtual ~antialiasing() = default;

        virtual int sample_count() const = 0;
        virtual aa_sample sample(int sample_index, random_source& rng) const = 0;

        virtual double sample_scale() const {
            return 1.0 / sample_count();
        }
};

class antialiasing_off final : public antialiasing {
    public:
        int sample_count() const override {
            return 1;
        }

        aa_sample sample(int sample_index, random_source& rng) const override {
            (void)sample_index;
            (void)rng;
            return {};
        }
};

class random_antialiasing final : public antialiasing {
    public:
        explicit random_antialiasing(int samples_per_pixel)
            : samples_per_pixel(std::max(1, samples_per_pixel)) {}

        int sample_count() const override {
            return samples_per_pixel;
        }

        aa_sample sample(int sample_index, random_source& rng) const override {
            (void)sample_index;
            return {rng.random_double(-0.5, 0.5), rng.random_double(-0.5, 0.5), 1.0};
        }

    private:
        int samples_per_pixel;
};

inline int exact_square_grid_axis(int samples_per_pixel);
inline bool is_square_sample_count(int samples_per_pixel);

class grid_antialiasing final : public antialiasing {
    public:
        explicit grid_antialiasing(int requested_samples_per_pixel) {
            samples_per_axis = exact_square_grid_axis(requested_samples_per_pixel);
            samples_per_pixel = samples_per_axis * samples_per_axis;
        }

        int sample_count() const override {
            return samples_per_pixel;
        }

        aa_sample sample(int sample_index, random_source& rng) const override {
            (void)rng;
            const int u_index = sample_index % samples_per_axis;
            const int v_index = sample_index / samples_per_axis;
            return {
                (static_cast<double>(u_index) + 0.5) / samples_per_axis - 0.5,
                (static_cast<double>(v_index) + 0.5) / samples_per_axis - 0.5,
                1.0
            };
        }

    private:
        int samples_per_pixel;
        int samples_per_axis;
};

class jittered_grid_antialiasing final : public antialiasing {
    public:
        explicit jittered_grid_antialiasing(int requested_samples_per_pixel) {
            samples_per_axis = exact_square_grid_axis(requested_samples_per_pixel);
            samples_per_pixel = samples_per_axis * samples_per_axis;
        }

        int sample_count() const override {
            return samples_per_pixel;
        }

        aa_sample sample(int sample_index, random_source& rng) const override {
            const int u_index = sample_index % samples_per_axis;
            const int v_index = sample_index / samples_per_axis;
            return {
                (static_cast<double>(u_index) + rng.random_double()) / samples_per_axis - 0.5,
                (static_cast<double>(v_index) + rng.random_double()) / samples_per_axis - 0.5,
                1.0
            };
        }

    private:
        int samples_per_pixel;
        int samples_per_axis;
};

inline int exact_square_grid_axis(int samples_per_pixel) {
    if (samples_per_pixel < 1) {
        throw std::invalid_argument("grid sampling requires a positive square sample count");
    }

    const int axis = static_cast<int>(std::sqrt(samples_per_pixel));
    if (axis * axis != samples_per_pixel) {
        throw std::invalid_argument("grid sampling requires a perfect-square sample count");
    }

    return axis;
}

inline bool is_square_sample_count(int samples_per_pixel) {
    if (samples_per_pixel < 1) {
        return false;
    }

    const int axis = static_cast<int>(std::sqrt(samples_per_pixel));
    return axis * axis == samples_per_pixel;
}

inline std::shared_ptr<const antialiasing> make_sampling_strategy(
    sampling_pattern pattern,
    int parameter
) {
    switch (pattern) {
        case sampling_pattern::aa_off:
            return std::make_shared<antialiasing_off>();
        case sampling_pattern::random:
            return std::make_shared<random_antialiasing>(parameter);
        case sampling_pattern::grid:
            return std::make_shared<grid_antialiasing>(parameter);
        case sampling_pattern::jittered_grid:
            return std::make_shared<jittered_grid_antialiasing>(parameter);
        default:
            return std::make_shared<antialiasing_off>();
    }
}

#endif
