#ifndef RT_COMMON_H
#define RT_COMMON_H

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

#include "ray.h"
#include "vec3.h"
#include "color.h"
#include "interval.h"


#endif