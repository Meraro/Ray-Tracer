#include "rt_common.h"

#include "antialiasing.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "camera.h"
#include "vec3.h"
#include <memory>

int main() {
    hittable_list world;

    auto material_ground = 
        make_shared<lambertian>(color(0.8, 0.8, 0));
    auto material_center = 
        make_shared<lambertian>(color(0.1, 0.2, 0.5));
    auto material_left = 
        make_shared<metal>(color(0.8, 0.8, 0.8), 0.2);
    auto material_right = 
        make_shared<metal>(color(0.8, 0.6, 0.2), 0.8);

    world.add(make_shared<sphere>(point3(0, -100.5, -1.0), 100, material_ground));
    world.add(make_shared<sphere>(point3(0, 0, -1.2), 0.5, material_center));
    world.add(make_shared<sphere>(point3(-1, 0, -1), 0.5, material_left));
    world.add(make_shared<sphere>(point3(1, 0, -1), 0.5, material_right));

    double aspect_ratio = 16.0 / 9.0;
    int image_width = 800;
    std::shared_ptr<const antialiasing> aa_strategy = std::make_shared<random_antialiasing>(50);
    
    camera cam(aspect_ratio, image_width);
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.max_depth = 50;
    cam.set_antialiasing(aa_strategy);
    // aa_strategy = std::make_shared<random_antialiasing>(50);
    // aa_strategy = std::make_shared<grid_antialiasing>(4);
    // aa_strategy = std::make_shared<jittered_grid_antialiasing>(4);

    
    cam.render(world);
}
