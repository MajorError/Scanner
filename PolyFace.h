/* 
 * File:   PolyFace.h
 * Author: majorerror
 *
 * Created on 08 December 2009, 18:16
 */

#ifndef _POLYFACE_H
#define	_POLYFACE_H

#include "Edge.h"
#include <cvd/image.h>
#include <cvd/rgb.h>
#include <cvd/byte.h>
#include <TooN/TooN.h>
#include <TooN/se3.h>

using namespace CVD;
using namespace TooN;

class PolyFace {
public:
    PolyFace() : p1( NULL ), p2( NULL ), p3( NULL ) {};
    PolyFace( Point* a, Point* b, Point* c );
    virtual ~PolyFace() {};
    Point* getP1();
    Vector<2> getP1Coord();
    Point* getP2();
    Vector<2> getP2Coord();
    Point* getP3();
    Vector<2> getP3Coord();
    void setTexture( Image< Rgb< byte > > t, SE3<> vp );
    SubImage< Rgb< byte > >& getTexture();
    SE3<>& getTextureViewpoint();
    Vector<3> getFaceCentre();
    bool operator()( PolyFace* a, PolyFace* b ) const;
protected:
    Point* p1;
    Point* p2;
    Point* p3;
    SE3<> textureViewpoint;
    Image< Rgb< byte > > texture;
    double scale;
    double offset;

};

#endif	/* _POLYFACE_H */

