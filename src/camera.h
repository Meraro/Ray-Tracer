#ifndef CAMERA_H
#define CAMERA_H

#include "color.h"
#include "constants.h"
#include "hittable.h"
#include "interval.h"
#include "vec3.h"
#include <iostream>

class camera {
    public:
        double  aspect_ratio = 16.0 / 9.0;
        int     image_width = 800;

        void render(const hittable& world) {
            initialize();

            std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

            for (int j=0 ; j<image_height ; ++j) {
                std::clog << "\rScanlines remaining: " << image_height-j << " " << std::flush;
                for (int i=0 ; i<image_width ; ++i) {
                    auto pixel_center = pixel00_loc + (i*pixel_delta_u + j*pixel_delta_v);
                    auto ray_direction = pixel_center - center;
                    ray r(center, ray_direction);

                    color pixel_color = ray_color(r, world);
                    write_color(std::cout, pixel_color);
                }
            }

            std::clog << "\rDone.                                   \n";
        }
    
    private:
        int image_height;
        point3 center;
        point3 pixel00_loc;
        vec3 pixel_delta_v;
        vec3 pixel_delta_u;
        
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

        color ray_color(const ray& r, const hittable& world) const {
            hit_record rec;

            if (world.hit(r, interval(0, infinity), rec)) {
                return 0.5 * (rec.normal + color(1, 1, 1));
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