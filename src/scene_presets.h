#ifndef SCENE_PRESETS_H
#define SCENE_PRESETS_H

#include "bvh.h"
#include "camera.h"
#include "constant_medium.h"
#include "hittable_list.h"
#include "material.h"
#include "quad.h"
#include "render_config.h"
#include "sphere.h"
#include "texture.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

enum class scene_id {
    bouncing_spheres = 1,
    checkered_spheres,
    earth,
    perlin_spheres,
    quads,
    simple_light,
    cornell_box,
    cornell_smoke,
    final_scene,
};

struct scene_preset {
    std::string name;
    camera cam;
    std::shared_ptr<hittable_list> geometry;
    scene_settings settings;
    std::uint64_t scene_seed = 1;
};

inline scene_preset make_scene_preset(
    std::string name,
    camera cam,
    std::shared_ptr<hittable_list> geometry,
    scene_settings settings,
    std::uint64_t scene_seed = 1
) {
    return {std::move(name), std::move(cam), std::move(geometry), std::move(settings), scene_seed};
}

inline scene_preset bouncing_spheres(std::uint64_t scene_seed = 1) {
    random_source scene_rng(scene_seed);
    auto world = make_shared<hittable_list>();

    auto checker = std::make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9,.9,.9));
    world->add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(checker)));
    
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = scene_rng.random_double();
            point3 center(a + 0.9*scene_rng.random_double(), 0.2, b + 0.9*scene_rng.random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    auto albedo = color::random(scene_rng) * color::random(scene_rng);
                    sphere_material = make_shared<lambertian>(albedo);
                    auto center2 = center + vec3(0, scene_rng.random_double(0, 0.5), 0);
                    world->add(make_shared<sphere>(center, center2, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    auto albedo = color::random(0.5, 1, scene_rng);
                    auto fuzz = scene_rng.random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world->add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    sphere_material = make_shared<dielectric>(1.5);
                    world->add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world->add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world->add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world->add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    camera cam;
    cam.vfov = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat = point3(0,0,0);
    cam.vup = vec3(0,1,0);
    cam.defocus_angle = 0.6;
    cam.focus_dist = 10.0;

    scene_settings settings;
    settings.aspect_ratio = 16.0 / 9.0;
    settings.image_width = 1200;
    settings.max_depth = 50;
    settings.background = color(0.70, 0.80, 1.00);
    settings.default_acceleration = acceleration_structure::bvh;
    settings.default_sampling_strategy = std::make_shared<random_antialiasing>(5);

    return make_scene_preset("bouncing_spheres", cam, world, settings, scene_seed);
}

inline scene_preset checkered_spheres(std::uint64_t scene_seed = 1) {
    auto world = make_shared<hittable_list>();
    auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));

    world->add(make_shared<sphere>(point3(0,-10, 0), 10, make_shared<lambertian>(checker)));
    world->add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

    camera cam;
    cam.vfov = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat = point3(0,0,0);
    cam.vup = vec3(0,1,0);
    cam.defocus_angle = 0;

    scene_settings settings;
    settings.aspect_ratio = 16.0 / 9.0;
    settings.image_width = 800;
    settings.max_depth = 50;
    settings.background = color(0.70, 0.80, 1.00);
    settings.default_sampling_strategy = make_shared<random_antialiasing>(10);

    return make_scene_preset("checkered_spheres", cam, world, settings, scene_seed);
}

inline scene_preset earth(std::uint64_t scene_seed = 1) {
    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    auto world = make_shared<hittable_list>(make_shared<sphere>(point3(0,0,0), 2, earth_surface));

    camera cam;
    cam.vfov = 20;
    cam.lookfrom = point3(0,0,12);
    cam.lookat = point3(0,0,0);
    cam.vup = vec3(0,1,0);
    cam.defocus_angle = 0;

    scene_settings settings;
    settings.aspect_ratio = 16.0 / 9.0;
    settings.image_width = 400;
    settings.max_depth = 50;
    settings.background = color(0.70, 0.80, 1.00);
    settings.default_sampling_strategy = make_shared<random_antialiasing>(50);

    return make_scene_preset("earth", cam, world, settings, scene_seed);
}

inline scene_preset perlin_spheres(std::uint64_t scene_seed = 1) {
    random_source scene_rng(scene_seed);
    auto world = make_shared<hittable_list>();
    auto pertext = make_shared<noise_texture>(4, scene_rng);
    world->add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));
    world->add(make_shared<sphere>(point3(0,2,0), 2, make_shared<lambertian>(pertext)));

    camera cam;
    cam.vfov = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat = point3(0,0,0);
    cam.vup = vec3(0,1,0);
    cam.defocus_angle = 0;

    scene_settings settings;
    settings.aspect_ratio = 16.0 / 9.0;
    settings.image_width = 400;
    settings.max_depth = 50;
    settings.background = color(0.70, 0.80, 1.00);
    settings.default_sampling_strategy = make_shared<random_antialiasing>(50);

    return make_scene_preset("perlin_spheres", cam, world, settings, scene_seed);
}

inline scene_preset quads(std::uint64_t scene_seed = 1) {
    auto world = make_shared<hittable_list>();

    auto left_red = make_shared<lambertian>(color(1.0, 0.2, 0.2));
    auto back_green = make_shared<lambertian>(color(0.2, 1.0, 0.2));
    auto right_blue = make_shared<lambertian>(color(0.2, 0.2, 1.0));
    auto upper_orange = make_shared<lambertian>(color(1.0, 0.5, 0.0));
    auto lower_teal = make_shared<lambertian>(color(0.2, 0.8, 0.8));

    world->add(make_shared<quad>(point3(-3,-2, 5), vec3(0, 0,-4), vec3(0, 4, 0), left_red));
    world->add(make_shared<quad>(point3(-2,-2, 0), vec3(4, 0, 0), vec3(0, 4, 0), back_green));
    world->add(make_shared<quad>(point3( 3,-2, 1), vec3(0, 0, 4), vec3(0, 4, 0), right_blue));
    world->add(make_shared<quad>(point3(-2, 3, 1), vec3(4, 0, 0), vec3(0, 0, 4), upper_orange));
    world->add(make_shared<quad>(point3(-2,-3, 5), vec3(4, 0, 0), vec3(0, 0,-4), lower_teal));

    camera cam;
    cam.vfov = 80;
    cam.lookfrom = point3(0,0,9);
    cam.lookat = point3(0,0,0);
    cam.vup = vec3(0,1,0);
    cam.defocus_angle = 0;

    scene_settings settings;
    settings.aspect_ratio = 1.0;
    settings.image_width = 400;
    settings.max_depth = 50;
    settings.background = color(0.70, 0.80, 1.00);
    settings.default_sampling_strategy = make_shared<random_antialiasing>(50);

    return make_scene_preset("quads", cam, world, settings, scene_seed);
}

inline scene_preset simple_light(std::uint64_t scene_seed = 1) {
    random_source scene_rng(scene_seed);
    auto world = make_shared<hittable_list>();
    auto pertext = make_shared<noise_texture>(4, scene_rng);
    world->add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));
    world->add(make_shared<sphere>(point3(0,2,0), 2, make_shared<lambertian>(pertext)));

    auto difflight = make_shared<diffuse_light>(color(4,4,4));
    world->add(make_shared<sphere>(point3(0,7,0), 2, difflight));
    world->add(make_shared<quad>(point3(3,1,-2), vec3(2,0,0), vec3(0,2,0), difflight));

    camera cam;
    cam.vfov = 20;
    cam.lookfrom = point3(26,3,6);
    cam.lookat = point3(0,2,0);
    cam.vup = vec3(0,1,0);
    cam.defocus_angle = 0;

    scene_settings settings;
    settings.aspect_ratio = 16.0 / 9.0;
    settings.image_width = 400;
    settings.max_depth = 50;
    settings.background = color(0,0,0);
    settings.default_sampling_strategy = make_shared<random_antialiasing>(100);

    return make_scene_preset("simple_light", cam, world, settings, scene_seed);
}

inline scene_preset cornell_box(std::uint64_t scene_seed = 1) {
    auto world = make_shared<hittable_list>();

    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));

    world->add(make_shared<quad>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green));
    world->add(make_shared<quad>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red));
    world->add(make_shared<quad>(point3(343, 554, 332), vec3(-130,0,0), vec3(0,0,-105), light));
    world->add(make_shared<quad>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white));
    world->add(make_shared<quad>(point3(555,555,555), vec3(-555,0,0), vec3(0,0,-555), white));
    world->add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    shared_ptr<hittable> box1 = box(point3(0,0,0), point3(165,330,165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265,0,295));
    world->add(box1);

    shared_ptr<hittable> box2 = box(point3(0,0,0), point3(165,165,165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130,0,65));
    world->add(box2);

    camera cam;
    cam.vfov = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat = point3(278, 278, 0);
    cam.vup = vec3(0,1,0);
    cam.defocus_angle = 0;

    scene_settings settings;
    settings.aspect_ratio = 1.0;
    settings.image_width = 600;
    settings.max_depth = 50;
    settings.background = color(0,0,0);
    settings.default_sampling_strategy = make_shared<random_antialiasing>(100);

    return make_scene_preset("cornell_box", cam, world, settings, scene_seed);
}

inline scene_preset cornell_smoke(std::uint64_t scene_seed = 1) {
    auto world = make_shared<hittable_list>();

    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(7, 7, 7));

    world->add(make_shared<quad>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green));
    world->add(make_shared<quad>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red));
    world->add(make_shared<quad>(point3(113,554,127), vec3(330,0,0), vec3(0,0,305), light));
    world->add(make_shared<quad>(point3(0,555,0), vec3(555,0,0), vec3(0,0,555), white));
    world->add(make_shared<quad>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white));
    world->add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    shared_ptr<hittable> box1 = box(point3(0,0,0), point3(165,330,165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265,0,295));

    shared_ptr<hittable> box2 = box(point3(0,0,0), point3(165,165,165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130,0,65));

    world->add(make_shared<constant_medium>(box1, 0.01, color(0,0,0), 1));
    world->add(make_shared<constant_medium>(box2, 0.01, color(1,1,1), 2));

    camera cam;
    cam.vfov = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat = point3(278, 278, 0);
    cam.vup = vec3(0,1,0);
    cam.defocus_angle = 0;

    scene_settings settings;
    settings.aspect_ratio = 1.0;
    settings.image_width = 600;
    settings.max_depth = 50;
    settings.background = color(0,0,0);
    settings.default_sampling_strategy = make_shared<random_antialiasing>(200);

    return make_scene_preset("cornell_smoke", cam, world, settings, scene_seed);
}

inline scene_preset final_scene(std::uint64_t scene_seed = 1) {
    random_source scene_rng(scene_seed);
    auto world = make_shared<hittable_list>();

    hittable_list boxes1;
    auto ground = make_shared<lambertian>(color(0.48, 0.83, 0.53));

    const int boxes_per_side = 20;
    for (int i = 0; i < boxes_per_side; ++i) {
        for (int j = 0; j < boxes_per_side; ++j) {
            const auto w = 100.0;
            const auto x0 = -1000.0 + i * w;
            const auto z0 = -1000.0 + j * w;
            const auto y0 = 0.0;
            const auto x1 = x0 + w;
            const auto y1 = scene_rng.random_double(1, 101);
            const auto z1 = z0 + w;

            boxes1.add(box(point3(x0, y0, z0), point3(x1, y1, z1), ground));
        }
    }

    world->add(make_shared<bvh_node>(boxes1, scene_rng));

    auto light = make_shared<diffuse_light>(color(7, 7, 7));
    world->add(make_shared<quad>(point3(123, 554, 147), vec3(300, 0, 0), vec3(0, 0, 265), light));

    const auto center1 = point3(400, 400, 200);
    const auto center2 = center1 + vec3(30, 0, 0);
    auto sphere_material = make_shared<lambertian>(color(0.7, 0.3, 0.1));
    world->add(make_shared<sphere>(center1, center2, 50, sphere_material));

    world->add(make_shared<sphere>(point3(260, 150, 45), 50, make_shared<dielectric>(1.5)));
    world->add(make_shared<sphere>(
        point3(0, 150, 145),
        50,
        make_shared<metal>(color(0.8, 0.8, 0.9), 1.0)
    ));

    auto boundary = make_shared<sphere>(point3(360, 150, 145), 70, make_shared<dielectric>(1.5));
    world->add(boundary);
    world->add(make_shared<constant_medium>(boundary, 0.2, color(0.2, 0.4, 0.9), 3));

    boundary = make_shared<sphere>(point3(0, 0, 0), 5000, make_shared<dielectric>(1.5));
    world->add(make_shared<constant_medium>(boundary, 0.0001, color(1, 1, 1), 4));

    auto emat = make_shared<lambertian>(make_shared<image_texture>("earthmap.jpg"));
    world->add(make_shared<sphere>(point3(400, 200, 400), 100, emat));

    auto pertext = make_shared<noise_texture>(0.2, scene_rng);
    world->add(make_shared<sphere>(point3(220, 280, 300), 80, make_shared<lambertian>(pertext)));

    hittable_list boxes2;
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    const int sphere_count = 1000;
    for (int j = 0; j < sphere_count; ++j) {
        boxes2.add(make_shared<sphere>(point3::random(0, 165, scene_rng), 10, white));
    }

    world->add(make_shared<translate>(
        make_shared<rotate_y>(
            make_shared<bvh_node>(boxes2, scene_rng),
            15
        ),
        vec3(-100, 270, 395)
    ));

    camera cam;
    cam.vfov = 40;
    cam.lookfrom = point3(478, 278, -600);
    cam.lookat = point3(278, 278, 0);
    cam.vup = vec3(0, 1, 0);
    cam.defocus_angle = 0;

    scene_settings settings;
    settings.aspect_ratio = 1.0;
    settings.image_width = 800;
    settings.max_depth = 40;
    settings.background = color(0, 0, 0);
    settings.default_acceleration = acceleration_structure::bvh;
    settings.default_sampling_strategy = make_shared<random_antialiasing>(100);

    return make_scene_preset("final_scene", cam, world, settings, scene_seed);
}

inline scene_preset make_scene(scene_id id, std::uint64_t scene_seed = 1) {
    switch (id) {
        case scene_id::bouncing_spheres:  return bouncing_spheres(scene_seed);
        case scene_id::checkered_spheres: return checkered_spheres(scene_seed);
        case scene_id::earth:             return earth(scene_seed);
        case scene_id::perlin_spheres:    return perlin_spheres(scene_seed);
        case scene_id::quads:             return quads(scene_seed);
        case scene_id::simple_light:      return simple_light(scene_seed);
        case scene_id::cornell_box:       return cornell_box(scene_seed);
        case scene_id::cornell_smoke:     return cornell_smoke(scene_seed);
        case scene_id::final_scene:       return final_scene(scene_seed);
        default:                          return cornell_smoke(scene_seed);
    }
}

#endif
