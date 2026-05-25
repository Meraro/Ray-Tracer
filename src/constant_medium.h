#ifndef CONSTANT_MEDIUM_H
#define CONSTANT_MEDIUM_H

#include "hittable.h"
#include "material.h"
#include "texture.h"

#include <cstdint>
#include <cstring>

class constant_medium : public hittable {
  public:
    constant_medium(
        shared_ptr<hittable> boundary,
        double density,
        shared_ptr<texture> tex,
        std::uint64_t random_stream_id = 0
    )
      : boundary(boundary), neg_inv_density(-1/density),
        phase_function(make_shared<isotropic>(tex)),
        random_stream_id(random_stream_id)
    {}

    constant_medium(
        shared_ptr<hittable> boundary,
        double density,
        const color& albedo,
        std::uint64_t random_stream_id = 0
    )
      : boundary(boundary), neg_inv_density(-1/density),
        phase_function(make_shared<isotropic>(albedo)),
        random_stream_id(random_stream_id)
    {}

    bool hit(const ray& r, interval ray_t, hit_record& rec, const hit_context& context) const override {
        hit_record rec1, rec2;

        if (!boundary->hit(r, interval::universe, rec1, context))
            return false;

        if (!boundary->hit(r, interval(rec1.t+0.0001, infinity), rec2, context))
            return false;

        if (rec1.t < ray_t.min) rec1.t = ray_t.min;
        if (rec2.t > ray_t.max) rec2.t = ray_t.max;

        if (rec1.t >= rec2.t)
            return false;

        if (rec1.t < 0)
            rec1.t = 0;

        auto ray_length = r.direction().length();
        auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;
        random_source medium_rng(medium_sample_seed(r, context));
        auto hit_distance = neg_inv_density * std::log(medium_rng.random_double());

        if (hit_distance > distance_inside_boundary)
            return false;

        rec.t = rec1.t + hit_distance / ray_length;
        rec.p = r.at(rec.t);

        rec.normal = vec3(1,0,0);  // arbitrary
        rec.front_face = true;     // also arbitrary
        rec.u = 0.0;
        rec.v = 0.0;
        rec.mat = phase_function;

        return true;
    }

    aabb bounding_box() const override { return boundary->bounding_box(); }

  private:
    shared_ptr<hittable> boundary;
    double neg_inv_density;
    shared_ptr<material> phase_function;
    std::uint64_t random_stream_id;

    static std::uint64_t double_seed(double value) {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        return mix_seed(bits);
    }

    std::uint64_t medium_sample_seed(const ray& r, const hit_context& context) const {
        auto seed = combine_seed(context.sampling_seed, context.pixel_index);
        seed = combine_seed(seed, static_cast<std::uint64_t>(context.sample_index));
        seed = combine_seed(seed, static_cast<std::uint64_t>(context.depth));
        seed = combine_seed(seed, random_stream_id);
        seed = combine_seed(seed, double_seed(r.origin().x()));
        seed = combine_seed(seed, double_seed(r.origin().y()));
        seed = combine_seed(seed, double_seed(r.origin().z()));
        seed = combine_seed(seed, double_seed(r.direction().x()));
        seed = combine_seed(seed, double_seed(r.direction().y()));
        seed = combine_seed(seed, double_seed(r.direction().z()));
        seed = combine_seed(seed, double_seed(r.time()));
        return seed;
    }
};

#endif
