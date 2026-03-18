#ifndef COLOR_H
#define COLOR_H

#include "interval.h"
#include "vec3.h"

using color = vec3;

inline void write_color(std::ostream& out, const color& pixel_color) {
   auto r = pixel_color.x();
   auto g = pixel_color.y();
   auto b = pixel_color.z();
   
   static const interval intensity(0.000, 0.999);
   int rbyte = static_cast<int>(256 * intensity.clamp(r));
   int gbyte = static_cast<int>(256 * intensity.clamp(g));
   int bbyte = static_cast<int>(256 * intensity.clamp(b));

   out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}

const auto white = color(1.0, 1.0, 1.0);
const auto blue = color(0.5, 0.7, 1.0);

#endif