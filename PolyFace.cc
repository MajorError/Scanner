/* 
 * File:   PolyFace.cc
 * Author: majorerror
 * 
 * Created on 08 December 2009, 18:16
 */

#include <algorithm>
#include "PolyFace.h"

using namespace std;

PolyFace::PolyFace( Point* a, Point* b, Point* c ) : 
        p1( min( a, min( b, c ) ) ),
        p3( max( a, max( b, c ) ) ) {
    p2 = p1 == a ? (p3 == b ? c : b) : (p3 == a ? b : a);
};


Point* PolyFace::getP1() {
    return p1;
};

Point* PolyFace::getP2() {
    return p2;
};

Point* PolyFace::getP3() {
    return p3;
};

bool PolyFace::operator()( PolyFace* a, PolyFace* p ) const {
    return a->p1 < p->p1 || a->p2 < p->p2 || a->p3 < p->p3;
};