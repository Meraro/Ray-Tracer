#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <iostream>

class vec3 {
    public:
        double e[3]{0, 0, 0};

        constexpr vec3() = default;
        constexpr vec3(double e0, double e1, double e2) : e{e0, e1, e2} {}

        constexpr double x() const { return e[0]; }
        constexpr double y() const { return e[1]; }
        constexpr double z() const { return e[2]; }

        constexpr vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
        constexpr double operator[](int i) const { return e[i]; }
        constexpr double& operator[](int i) { return e[i]; }

        constexpr vec3& operator+=(const vec3& v) {
            e[0] += v.e[0];
            e[1] += v.e[1];
            e[2] += v.e[2];

            return *this;
        }

        constexpr vec3& operator*=(double t) {
            e[0] *= t;
            e[1] *= t;
            e[2] *= t;

            return *this;
        }

        constexpr vec3& operator/=(double t) {
            return *this *=  1/t;
        }

        double length() const {
            return std::sqrt(length_squared());
        }

        constexpr double length_squared() const {
            return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
        }
};

// Alias point3 as vec3
using point3 = vec3;

// Vector Utility Functions

inline std::ostream& operator<<(std::ostream& out, const vec3& v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

constexpr vec3 operator+(const vec3 &u, const vec3 &v) {
    return vec3(u[0]+v[0], u[1]+v[1], u[2]+v[2]);
}

constexpr vec3 operator-(const vec3 &u, const vec3 &v) {
    return vec3(u[0]-v[0], u[1]-v[1], u[2]-v[2]);
}

constexpr vec3 operator*(const vec3& u, const vec3 &v) {
    return vec3(u[0]*v[0], u[1]*v[1], u[2]*v[2]);
}

constexpr vec3 operator*(double t, const vec3 &v) {
    return vec3(t*v[0], t*v[1], t*v[2]);
}

constexpr vec3 operator*(const vec3 &v, double t) {
    return t * v;
}

constexpr vec3 operator/(const vec3 &v, double t) {
    return (1/t) * v;
}

constexpr double dot(const vec3& u, const vec3& v) {
    return u[0] * v[0]
         + u[1] * v[1]
         + u[2] * v[2];
}

constexpr vec3 cross(const vec3& u, const vec3& v) {
    return vec3(u[1] * v[2] - u[2] * v[1],
                u[2] * v[0] - u[0] * v[2],
                u[0] * v[1] - u[1] * v[0]);
}

inline vec3 unit_vector(const vec3& v) {
    return v / v.length();
}

#endif