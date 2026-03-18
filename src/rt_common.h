#ifndef RT_COMMON_H
#define RT_COMMON_H

#include <random>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include "constants.h"

using std::make_shared;
using std::shared_ptr;

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double random_double() {
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
}

inline double random_double(double min, double max) {
    return min + (max - min)*random_double();
}

#include "ray.h"
#include "vec3.h"
#include "color.h"
#include "interval.h"


#endif