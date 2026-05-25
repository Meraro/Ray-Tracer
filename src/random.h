#ifndef RANDOM_H
#define RANDOM_H

#include <cstdint>
#include <random>

class random_source {
public:
    explicit random_source(std::uint64_t seed = 0) {
        seed_generator(seed);
    }

    double random_double() {
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        return distribution(generator);
    }

    double random_double(double min, double max) {
        return min + (max - min) * random_double();
    }

    int random_int(int min, int max) {
        return static_cast<int>(random_double(min, max + 1));
    }

private:
    std::mt19937 generator;

    void seed_generator(std::uint64_t seed) {
        std::seed_seq sequence{
            static_cast<std::uint32_t>(seed),
            static_cast<std::uint32_t>(seed >> 32)
        };
        generator.seed(sequence);
    }
};

inline std::uint64_t mix_seed(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

inline std::uint64_t combine_seed(std::uint64_t seed, std::uint64_t stream) {
    return mix_seed(seed ^ mix_seed(stream));
}

#endif
