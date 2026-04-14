#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include <memory>
#include "rtw_stb_image.h"
#include "perlin.h"

class texture {
public:
    virtual ~texture() = default;

    virtual color value(double u, double v, const point3& p) const = 0; 
};

class solid_color : public texture {
public:
    solid_color(const color& albedo) : albedo(albedo) {}

    solid_color(double r, double g, double b) : solid_color(color(r, g, b)) {};

    color value(double u, double v, const point3& p) const override { return albedo; }

private:
    color albedo;
};

class checker_texture : public texture {
public:
    checker_texture(double scale, std::shared_ptr<texture> even, std::shared_ptr<texture> odd)
        : inv_scale(1.0 / scale), even(even), odd(odd) {}

    checker_texture(double scale, const color& c1, const color& c2)
        : checker_texture(scale, std::make_shared<solid_color>(c1), std::make_shared<solid_color>(c2)) {}
    
    color value(double u, double v, const point3& p) const override {
        auto xInt = static_cast<int>(std::floor(inv_scale * p.x()));
        auto yInt = static_cast<int>(std::floor(inv_scale * p.y()));
        auto zInt = static_cast<int>(std::floor(inv_scale * p.z()));

        bool isEven = (xInt + yInt + zInt) % 2 == 0;

        return isEven ? even->value(u, v, p) : odd->value(u, v, p);
    }

private:
    double inv_scale;
    std::shared_ptr<texture> even;
    std::shared_ptr<texture> odd;
};

class image_texture : public texture {
  public:
    image_texture(const char* filename) : image(filename) {};

    color value(double u, double v, const point3& p) const override {
        if (image.height() <= 0) return color(0, 1, 1);

        u = interval(0,1).clamp(u);
        v = 1.0 - interval(0,1).clamp(v);

        auto i = static_cast<int>(u * image.width());
        auto j = static_cast<int>(v * image.height());
        auto pixel = image.pixel_data(i, j);

        auto color_scale = 1.0 / 255.0;
        return color_scale * color(pixel[0], pixel[1], pixel[2]);
    }

  private:
    rtw_image image;
};

class noise_texture : public texture {
  public:
    noise_texture(double scale) : scale(scale) {}

    color value(double u, double v, const point3& p) const override {
        // return color(1, 1, 1) * 0.5 * (1.0 + noise.noise(scale * p));
        return color(1, 1, 1) * noise.turb(p, 7);
    }
  
  private:
    double scale;
    perlin noise;
};

#endif