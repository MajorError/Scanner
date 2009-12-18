/* 
 * File:   PolyFace.h
 * Author: majorerror
 *
 * Created on 08 December 2009, 18:16
 */

#ifndef _POLYFACE_H
#define	_POLYFACE_H

#include "Edge.h"
#include "ATANCamera.h"
#include <cvd/image.h>
#include <cvd/rgb.h>
#include <cvd/byte.h>
#include <TooN/TooN.h>
#include <TooN/se3.h>

using namespace CVD;
using namespace TooN;

// Forward decl - ugly, but it'll do for now
// TODO: Clean me!
class Environment {
public: ATANCamera* getCamera();
};

class PolyFace {
public:
    PolyFace() : p1( NULL ), p2( NULL ), p3( NULL ) {};
    PolyFace( Point* a, Point* b, Point* c );
    virtual ~PolyFace() {};
    Point* getP1();
    Vector<2> getP1Coord( Environment* env );
    Point* getP2();
    Vector<2> getP2Coord( Environment* env );
    Point* getP3();
    Vector<2> getP3Coord( Environment* env );
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

