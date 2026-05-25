#ifndef QUALITY_METRICS_H
#define QUALITY_METRICS_H

#include "image.h"

#include <cmath>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

struct ppm_reference {
    std::optional<image> pixels;
    std::string error;
};

struct quality_result {
    bool available = false;
    double mse = 0.0;
    double psnr_db = 0.0;
    std::string error;
};

inline double psnr_from_mse(double mse) {
    return mse == 0.0
        ? std::numeric_limits<double>::infinity()
        : 10.0 * std::log10(1.0 / mse);
}

namespace quality_detail {

inline bool read_ppm_token(std::istream& in, std::string& token) {
    while (in >> token) {
        if (!token.empty() && token[0] == '#') {
            std::string rest_of_line;
            std::getline(in, rest_of_line);
            continue;
        }

        return true;
    }

    return false;
}

inline bool read_ppm_int(std::istream& in, int& value) {
    std::string token;
    if (!read_ppm_token(in, token)) {
        return false;
    }

    std::istringstream parser(token);
    parser >> value;
    return !parser.fail();
}

} // namespace quality_detail

inline ppm_reference load_ppm_reference(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        return {std::nullopt, "failed_to_open_reference"};
    }

    std::string magic;
    if (!quality_detail::read_ppm_token(input, magic) || magic != "P3") {
        return {std::nullopt, "unsupported_reference_format"};
    }

    int width = 0;
    int height = 0;
    int max_value = 0;
    if (!quality_detail::read_ppm_int(input, width) ||
        !quality_detail::read_ppm_int(input, height) ||
        !quality_detail::read_ppm_int(input, max_value)) {
        return {std::nullopt, "invalid_reference_header"};
    }

    if (width <= 0 || height <= 0 || max_value <= 0) {
        return {std::nullopt, "invalid_reference_dimensions"};
    }

    image reference(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int r = 0;
            int g = 0;
            int b = 0;
            if (!quality_detail::read_ppm_int(input, r) ||
                !quality_detail::read_ppm_int(input, g) ||
                !quality_detail::read_ppm_int(input, b)) {
                return {std::nullopt, "invalid_reference_pixel_data"};
            }

            reference.pixel(x, y) = color(
                static_cast<double>(r) / max_value,
                static_cast<double>(g) / max_value,
                static_cast<double>(b) / max_value
            );
        }
    }

    return {std::move(reference), ""};
}

inline quality_result compare_to_reference(
    const image& rendered_linear,
    const image& reference_display
) {
    if (rendered_linear.width() != reference_display.width() ||
        rendered_linear.height() != reference_display.height()) {
        return {false, 0.0, 0.0, "reference_size_mismatch"};
    }

    double sum_squared_error = 0.0;
    const auto width = rendered_linear.width();
    const auto height = rendered_linear.height();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto rendered = color_to_display_rgb(rendered_linear.pixel(x, y));
            const auto reference = reference_display.pixel(x, y);

            const auto dr = rendered.x() - reference.x();
            const auto dg = rendered.y() - reference.y();
            const auto db = rendered.z() - reference.z();
            sum_squared_error += dr * dr + dg * dg + db * db;
        }
    }

    const auto component_count = static_cast<double>(width) * height * 3.0;
    const auto mse = sum_squared_error / component_count;
    return {true, mse, psnr_from_mse(mse), ""};
}

#endif
