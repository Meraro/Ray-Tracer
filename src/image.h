#ifndef IMAGE_H
#define IMAGE_H

#include "color.h"

#include <ostream>
#include <vector>

class image {
public:
    image() = default;

    image(int width, int height)
        : width_(width), height_(height), pixels_(static_cast<size_t>(width) * height) {}

    int width() const { return width_; }
    int height() const { return height_; }

    color& pixel(int x, int y) {
        return pixels_[index(x, y)];
    }

    const color& pixel(int x, int y) const {
        return pixels_[index(x, y)];
    }

    void write_ppm(std::ostream& out) const {
        out << "P3\n" << width_ << ' ' << height_ << "\n255\n";
        for (const auto& pixel_color : pixels_) {
            write_color(out, pixel_color);
        }
    }

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<color> pixels_;

    size_t index(int x, int y) const {
        return static_cast<size_t>(y) * width_ + x;
    }
};

#endif
