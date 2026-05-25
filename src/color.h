#ifndef COLOR_H
#define COLOR_H

#include "interval.h"
#include "vec3.h"

using color = vec3;

struct rgb8 {
   int r = 0;
   int g = 0;
   int b = 0;
};

inline double linear_to_gamma(double linear_component) {
   if (linear_component > 0) 
      return std::sqrt(linear_component);
   
   return 0;
}

inline rgb8 color_to_rgb8(const color& pixel_color) {
   auto r = pixel_color.x();
   auto g = pixel_color.y();
   auto b = pixel_color.z();
   
   r = linear_to_gamma(r);
   g = linear_to_gamma(g);
   b = linear_to_gamma(b);
   
   static const interval intensity(0.000, 0.999);
   return {
      static_cast<int>(256 * intensity.clamp(r)),
      static_cast<int>(256 * intensity.clamp(g)),
      static_cast<int>(256 * intensity.clamp(b))
   };
}

inline color color_to_display_rgb(const color& pixel_color) {
   const auto rgb = color_to_rgb8(pixel_color);
   return color(rgb.r / 255.0, rgb.g / 255.0, rgb.b / 255.0);
}

inline void write_color(std::ostream& out, const color& pixel_color) {
   const auto rgb = color_to_rgb8(pixel_color);
   out << rgb.r << ' ' << rgb.g << ' ' << rgb.b << '\n';
}

const auto white = color(1.0, 1.0, 1.0);
const auto blue = color(0.5, 0.7, 1.0);

#endif
