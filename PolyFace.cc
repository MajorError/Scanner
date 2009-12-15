/* 
 * File:   PolyFace.cc
 * Author: majorerror
 * 
 * Created on 08 December 2009, 18:16
 */

#include <algorithm>
#include "PolyFace.h"
#include "Point.h"

using namespace std;

PolyFace::PolyFace( Point* a, Point* b, Point* c ) : 
        p1( min( a, min( b, c ) ) ),
        p3( max( a, max( b, c ) ) ),
        textureViewpoint( SO3<>(), makeVector( numeric_limits<double>::max(), numeric_limits<double>::max(), numeric_limits<double>::max() ) ),
        texture( ImageRef( 640, 480 ), Rgb<byte>( 255, 0, 0 ) ){
    p2 = p1 == a ? (p3 == b ? c : b) : (p3 == a ? b : a);
};


Point* PolyFace::getP1() {
    return p1;
};

Vector<2> PolyFace::getP1Coord() {
    return makeVector( 0.1, 0.1 );
};

Point* PolyFace::getP2() {
    return p2;
};

Vector<2> PolyFace::getP2Coord() {
    return makeVector( 0.1, 0.9 );
};

Point* PolyFace::getP3() {
    return p3;
};

Vector<2> PolyFace::getP3Coord() {
    return makeVector( 0.9, 0.0 );
};

void PolyFace::setTexture( Image< Rgb< byte > > t, SE3<> vp ) {
    texture.copy_from( t );
    textureViewpoint = vp;
};

SubImage< Rgb< byte > >& PolyFace::getTexture() {
    return texture;
};

SE3<>& PolyFace::getTextureViewpoint() {
    return textureViewpoint;
};

Vector<3> PolyFace::getFaceCentre() {
    Vector<3> out = p1->getPosition() + p2->getPosition() + p3->getPosition();
    out /= 3;
    return out;
};

bool PolyFace::operator()( PolyFace* a, PolyFace* p ) const {
    return a->p1 < p->p1 || a->p2 < p->p2 || a->p3 < p->p3;
};