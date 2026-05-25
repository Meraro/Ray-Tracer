#ifndef WORLD_BUILDER_H
#define WORLD_BUILDER_H

#include "acceleration.h"
#include "bvh.h"
#include "hittable.h"
#include "hittable_list.h"
#include "random.h"

#include <cstdint>
#include <memory>

inline std::shared_ptr<hittable> build_world(
    const std::shared_ptr<hittable_list>& geometry,
    acceleration_structure acceleration,
    std::uint64_t build_seed
) {
    if (acceleration == acceleration_structure::bvh) {
        if (geometry->objects.empty()) {
            return geometry;
        }

        random_source build_rng(build_seed);
        return std::make_shared<bvh_node>(*geometry, build_rng);
    }

    return geometry;
}

#endif
