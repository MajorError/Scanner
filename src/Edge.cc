#include "Edge.h"

Point* Edge::getStart() {
    return from;
};

Point* Edge::getEnd() {
    return to;
};

void Edge::reset( Point* f, Point* t ) {
    from = f;
    to = t;
}