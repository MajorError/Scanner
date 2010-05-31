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
        p1( a < b ? (a < c ? a : c) : (b < c ? b : c) ),
        p3( a > b ? (a > c ? a : c) : (b > c ? b : c) ),
        textureViewpoint( SO3<>(), makeVector( numeric_limits<double>::max(), numeric_limits<double>::max(), numeric_limits<double>::max() ) ),
        texture( ImageRef( 640, 480 ), Rgb<byte>( 255, 0, 0 ) ),
        textureSource( &texture ), flipNormal( false ) {
    p2 = a < c ? (b < c ? (a < b ? b : a) : c) : (a < b ? a : (b < c ? c : b));
};

Point* PolyFace::getP1() {
    return p1;
};

Vector<2> PolyFace::getP1Coord( ATANCamera* cam ) {
    Vector<3> v = textureViewpoint.get_rotation().inverse() * (p1->getPosition() - textureViewpoint.get_translation());

    Vector<2> out = cam->Project( makeVector( v[0]/v[2], v[1]/v[2] ) );
    out[0] /= cam->GetImageSize()[0];
    out[1] /= cam->GetImageSize()[1];
    return out;
};

Point* PolyFace::getP2() {
    return p2;
};

Vector<2> PolyFace::getP2Coord( ATANCamera* cam ) {
    Vector<3> v = textureViewpoint.get_rotation().inverse() * (p2->getPosition() - textureViewpoint.get_translation());

    Vector<2> out = cam->Project( makeVector( v[0]/v[2], v[1]/v[2] ) );
    out[0] /= cam->GetImageSize()[0];
    out[1] /= cam->GetImageSize()[1];
    return out;
};

Point* PolyFace::getP3() {
    return p3;
};

Vector<2> PolyFace::getP3Coord( ATANCamera* cam ) {
    Vector<3> v = textureViewpoint.get_rotation().inverse() * (p3->getPosition() - textureViewpoint.get_translation());

    Vector<2> out = cam->Project( makeVector( v[0]/v[2], v[1]/v[2] ) );
    out[0] /= cam->GetImageSize()[0];
    out[1] /= cam->GetImageSize()[1];
    return out;
};

void PolyFace::testBoundsAndSetTexture( Image< Rgb< byte > >* t, SE3<> vp, ATANCamera* cam ) {
    SE3<> old = textureViewpoint;
    Vector<2> p1 = getP1Coord( cam );
    Vector<2> p2 = getP2Coord( cam );
    Vector<2> p3 = getP3Coord( cam );

    textureViewpoint = vp;
    p1 = getP1Coord( cam );
    p2 = getP2Coord( cam );
    p3 = getP3Coord( cam );
    if ( min( min( p1[0], p1[1] ), min( min( p2[0], p2[1] ), min( p3[0], p3[1] ) ) ) >= 0 &&
            max( max( p1[0], p1[1] ), max( max( p2[0], p2[1] ), max( p3[0], p3[1] ) ) ) <= 1 )
        setTexture( t, vp );
    else
        textureViewpoint = old;
};

void PolyFace::testAndSetTexture( Image< Rgb< byte > >& t, SE3<> vp, ATANCamera* cam ) {
    SE3<> old = textureViewpoint;
    // Don't bother taking the half, or the sqrt here
    Vector<2> p1 = getP1Coord( cam );
    Vector<2> p2 = getP2Coord( cam );
    Vector<2> p3 = getP3Coord( cam );
    // oldArea set to 0.0 if one of the points is out of bounds. This way we
    //   replace the bad texture frame ASAP.
    double oldArea = 0.0;
    if ( min( min( p1[0], p1[1] ), min( min( p2[0], p2[1] ), min( p3[0], p3[1] ) ) ) >= 0 &&
            max( max( p1[0], p1[1] ), max( max( p2[0], p2[1] ), max( p3[0], p3[1] ) ) ) <= 1 )
        oldArea = (p2 - p1) * (p3 - p1);

    textureViewpoint = vp;
    p1 = getP1Coord( cam );
    p2 = getP2Coord( cam );
    p3 = getP3Coord( cam );
    double newArea = (p2 - p1) * (p3 - p1);
    if ( (newArea < 0 ? -newArea : newArea) > (oldArea < 0 ? -oldArea : oldArea) &&
            min( min( p1[0], p1[1] ), min( min( p2[0], p2[1] ), min( p3[0], p3[1] ) ) ) >= 0 &&
            max( max( p1[0], p1[1] ), max( max( p2[0], p2[1] ), max( p3[0], p3[1] ) ) ) <= 1 )
        setTexture( t, vp );
    else
        textureViewpoint = old;
};

void PolyFace::setTexture( Image< Rgb< byte > >& t, SE3<> vp ) {
    texture.make_unique();
    texture.copy_from( t );
    textureViewpoint = vp;

    // Decide whether or not to flip the "natural" vertex normal
    Matrix<> rot = vp.get_rotation().get_matrix();
    Vector<3> look = makeVector( rot[0][2], rot[1][2], rot[2][2] );
    flipNormal = false;
    flipNormal = look * getFaceNormal() > 0;
};

void PolyFace::setTexture( Image< Rgb< byte > >* t, SE3<> vp ) {
    textureSource = t;
    setTexture( *t, vp );
};

Image< Rgb< byte > >& PolyFace::getTexture() {
    return texture;
};

Image< Rgb< byte > >* PolyFace::getTextureSource() {
    return textureSource;
};

SE3<>& PolyFace::getTextureViewpoint() {
    return textureViewpoint;
};

Vector<3> PolyFace::getFaceCentre() {
    Vector<3> out = p1->getPosition() + p2->getPosition() + p3->getPosition();
    out /= 3;
    return out;
};

Vector<3> PolyFace::getFaceNormal() {
    Vector<3> n = (p2->getPosition() - p1->getPosition()) ^ (p3->getPosition() - p1->getPosition());
    if ( flipNormal )
        n = (p3->getPosition() - p1->getPosition()) ^ (p2->getPosition() - p1->getPosition());
    return n / sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
};

bool PolyFace::hasFlippedNormal() {
    return flipNormal;
}

bool PolyFace::operator()( PolyFace* a, PolyFace* p ) const {
    return a->p1 < p->p1 || a->p2 < p->p2 || a->p3 < p->p3;
};