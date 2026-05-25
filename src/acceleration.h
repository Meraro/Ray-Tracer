#ifndef ACCELERATION_H
#define ACCELERATION_H

enum class acceleration_structure {
    plain,
    bvh,
};

inline const char* acceleration_name(acceleration_structure acceleration) {
    switch (acceleration) {
        case acceleration_structure::plain: return "plain";
        case acceleration_structure::bvh: return "bvh";
        default: return "unknown";
    }
}

#endif
