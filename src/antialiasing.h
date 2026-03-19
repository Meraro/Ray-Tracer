#ifndef ANTIALIASING_H
#define ANTIALIASING_H

#include "random.h"
#include <algorithm>
#include <memory>

struct aa_sample {
    double offset_u = 0.0;
    double offset_v = 0.0;
    double weight = 1.0;
};

class antialiasing {
    public:
        virtual ~antialiasing() = default;

        virtual int sample_count() const = 0;
        virtual aa_sample sample(int sample_index) const = 0;

        virtual double sample_scale() const {
            return 1.0 / sample_count();
        }
};

class antialiasing_off final : public antialiasing {
    public:
        int sample_count() const override {
            return 1;
        }

        aa_sample sample(int sample_index) const override {
            (void)sample_index;
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

        aa_sample sample(int sample_index) const override {
            (void)sample_index;
            return {random_double(-0.5, 0.5), random_double(-0.5, 0.5), 1.0};
        }

    private:
        int samples_per_pixel;
};

#endif
