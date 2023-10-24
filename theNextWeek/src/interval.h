#ifndef INTERVAL_H
#define INTERVAL_H

#include "constants.h"

class interval {
  public:
    double min, max;

    interval() : min(+infinity), max(-infinity) {}

    interval(double _min, double _max) : min(_min), max(_max) {}

    bool contains(double x) const { return min <= x && x <= max; }

    double size() const { return max - min; }

    interval expand(double delta) const {
        auto padding = delta / 2;
        return interval(min - padding, max + padding);
    }

    bool surrounds(double x) const { return min < x && x < max; }

    double clamp(double x) const {
        if (x < min)
            return min;
        if (x > max)
            return max;
        return x;
    }

    static const interval empty, universe;
};

const static interval empty(+infinity, -infinity);
const static interval universe(-infinity, +infinity);

#endif
