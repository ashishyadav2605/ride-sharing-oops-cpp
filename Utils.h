#ifndef UTILS_H
#define UTILS_H

#include <cmath>

struct Coord {
    double x, y;
    Coord(double x=0, double y=0): x(x), y(y) {}
};

inline double distance(const Coord &a, const Coord &b) {
    return std::sqrt((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y));
}

#endif
