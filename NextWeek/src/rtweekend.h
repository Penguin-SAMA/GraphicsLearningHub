#ifndef RTWEEKEND_H
#define RTWEEKEND_H

#include <cmath>
#include <cstdlib>
#include <memory>

// Usings

using std::make_shared;
using std::shared_ptr;
using std::sqrt;

// 常量

const double pi = 3.1415926535897932385;

// 实用函数

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double random_double() {
    return rand() / (RAND_MAX + 1.0);
}

inline double random_double(double min, double max) {
    return min + (max - min) * random_double();
}

#endif
