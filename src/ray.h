#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class ray {
    public:
        ray() = default;

        constexpr ray(const point3& origin, const vec3& direction) : orig(origin), dir(direction) {}

        constexpr const point3& origin() const { return orig; }
        constexpr const vec3& direction() const {return dir; }

        constexpr point3 at(double t) const {
            return orig + t*dir;
        }

    private:
        point3 orig;
        vec3 dir;
};

#endif