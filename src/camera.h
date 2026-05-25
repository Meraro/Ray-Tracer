#ifndef CAMERA_H
#define CAMERA_H

#include "antialiasing.h"
#include "constants.h"
#include "random.h"
#include "ray.h"
#include "vec3.h"

class camera {
public:
    double vfov = 90;
    point3 lookfrom = point3(0, 0, 0);
    point3 lookat = point3(0, 0, -1);
    vec3 vup = vec3(0, 1, 0);

    double defocus_angle = 0;   // Variation angle of rays through each pixel
    double focus_dist = 10;     // Distance from camera lookfrom point to plane of perfect focus

    camera() = default;

    void prepare(int image_width, double aspect_ratio) {
        this->image_width = image_width;
        this->image_height = static_cast<int>(image_width / aspect_ratio);
        this->image_height = image_height < 1 ? 1 : image_height;

        initialize();
    }

    int prepared_image_width() const { return image_width; }
    int prepared_image_height() const { return image_height; }

    ray get_ray(int i, int j, const aa_sample& sample, random_source& rng) const {
        auto pixel_sample = pixel00_loc
            + ((i + sample.offset_u) * pixel_delta_u)
            + ((j + sample.offset_v) * pixel_delta_v);
        
        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample(rng);
        auto ray_direction = pixel_sample - ray_origin;
        auto ray_time = rng.random_double();

        return ray(ray_origin, ray_direction, ray_time);
    }

private:
    int image_width = 0;
    int image_height;               // Rendered image height
    point3 center;                  // Camera center
    point3 pixel00_loc;             // Location of pixel 0, 0
    vec3 pixel_delta_u;             // Offset to pixel to the right
    vec3 pixel_delta_v;             // Offset to pixel below
    vec3 u, v, w;                   // Camera frame basis vectors
    vec3 defocus_disk_u;            // Defocus disk horizontal radius
    vec3 defocus_disk_v;            // Defocus disk vertical radius
    
    void initialize() {
        center = lookfrom;

        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta/2);
        auto viewport_height = 2 * h * focus_dist;
        auto viewport_width = viewport_height * (static_cast<double>(image_width) / image_height);

        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);
        
        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        vec3 viewport_u = viewport_width * u;       // Vector across viewport horizontal edge
        vec3 viewport_v = viewport_height * -v;     // Vector down viewport vertical edge

        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        auto viewport_upper_left = 
            center - (focus_dist * w) - 0.5*(viewport_u + viewport_v);
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    point3 defocus_disk_sample(random_source& rng) const {
        // Returns a random point in the camera defocus disk.
        auto p = random_in_unit_disk(rng);
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }
};

#endif
