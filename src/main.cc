#include "rt_common.h"

#include "antialiasing.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"

int main() {
    hittable_list world;

    world.add(std::make_shared<sphere>(point3(0, 0, -1), 0.5));
    world.add(make_shared<sphere>(point3(0, -100.5, -1), 100));

    double aspect_ratio = 16.0 / 9.0;
    int image_width = 800;

    camera cam(aspect_ratio, image_width);
    std::shared_ptr<const antialiasing> aa_strategy = std::make_shared<antialiasing_off>();
    // aa_strategy = std::make_shared<random_antialiasing>(50);
    // aa_strategy = std::make_shared<grid_antialiasing>(4);
    // aa_strategy = std::make_shared<jittered_grid_antialiasing>(4);

    cam.set_antialiasing(aa_strategy);
    
    cam.render(world);
}
