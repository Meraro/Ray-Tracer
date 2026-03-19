#ifndef CAMERA_H
#define CAMERA_H

#include "antialiasing.h"
#include "color.h"
#include "constants.h"
#include "hittable.h"
#include "interval.h"
#include "render_progress.h"
#include "vec3.h"
#include "material.h"

#include <iostream>
#include <memory>

class camera {
public:
    double  aspect_ratio    = 16.0 / 9.0;
    int     image_width     = 800;
    int     max_depth       = 100;    

    camera() = default;

    camera(double aspect_ratio, int image_width) : 
        aspect_ratio(aspect_ratio), image_width(image_width) {}

    void set_antialiasing(std::shared_ptr<const antialiasing> strategy) {
        antialiasing_strategy = strategy ? strategy : std::make_shared<antialiasing_off>();
    }

    void disable_antialiasing() {
        antialiasing_strategy = std::make_shared<antialiasing_off>();
    }

    void render(const hittable& world) {
        initialize();
        
        const double pixel_sample_scale = antialiasing_strategy->sample_scale();
        const int samples_per_pixel = antialiasing_strategy->sample_count();

        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
        render_progress progress(image_height);
        progress.update(0, true);

        for (int j=0 ; j<image_height ; ++j) {
            for (int i=0 ; i<image_width ; ++i) {
                color pixel_color(0, 0, 0);
                for (int sample_index=0 ; sample_index<samples_per_pixel ; ++sample_index) {
                    const auto sample = antialiasing_strategy->sample(sample_index);
                    ray r= get_ray(i, j, sample);
                    pixel_color += ray_color(r, max_depth, world) * sample.weight;
                } 
                write_color(std::cout, pixel_color * pixel_sample_scale);
            }

            progress.update(j + 1, false);
        }

        progress.done();
    }

private:
    int image_height;               // Rendered image height
    point3 center;                  // Camera center
    point3 pixel00_loc;             // Location of pixel 0, 0
    vec3 pixel_delta_v;             // Offset to pixel to the right
    vec3 pixel_delta_u;             // Offset to pixel below
    std::shared_ptr<const antialiasing> antialiasing_strategy 
        = std::make_shared<antialiasing_off>();
    
    void initialize() {
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = image_height < 1 ? 1 : image_height;

        center = point3(0, 0, 0);

        auto focal_length = 1.0;
        auto viewport_height = 2.0;
        auto viewport_width = viewport_height * (static_cast<double>(image_width) / image_height);

        auto viewport_u = vec3(viewport_width, 0, 0);
        auto viewport_v = vec3(0, -viewport_height, 0);

        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        auto viewport_upper_left = 
            center - vec3(0, 0, focal_length) - 0.5*(viewport_u + viewport_v);
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }

    ray get_ray(int i, int j, const aa_sample& sample) const {
        auto pixel_sample = pixel00_loc
            + ((i + sample.offset_u) * pixel_delta_u)
            + ((j + sample.offset_v) * pixel_delta_v);
        
        auto ray_origin = center;
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
    }

    color ray_color(const ray& r, int depth, const hittable& world) const {
        if (depth-- <= 0)
            return color(0, 0, 0);
        hit_record rec;

        if (world.hit(r, interval(0.001, infinity), rec)) {
            ray scattered;
            color attenuation;
            if (rec.mat->scatter(r, rec, attenuation, scattered)) 
                return attenuation * ray_color(scattered, depth, world);
            return color(0, 0, 0);
        }

        return render_sky(r);
    }

    color render_sky(const ray& r) const {
        vec3 unit_direction = unit_vector(r.direction());
        auto k = 0.5 * (unit_direction.y() + 1);
        return (1-k) * white + k * blue;
    }
};

#endif
