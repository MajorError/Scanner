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

class PolyFace {
public:
    PolyFace() : p1( NULL ), p2( NULL ), p3( NULL ) {};
    PolyFace( Point* a, Point* b, Point* c );
    virtual ~PolyFace() {};
    Point* getP1();
    virtual Vector<2> getP1Coord( ATANCamera* cam );
    Point* getP2();
    virtual Vector<2> getP2Coord( ATANCamera* cam );
    Point* getP3();
    virtual Vector<2> getP3Coord( ATANCamera* cam );
    void testBoundsAndSetTexture( Image< Rgb< byte > >* t, SE3<> vp, ATANCamera* cam );
    void testAndSetTexture( Image< Rgb< byte > >& t, SE3<> vp, ATANCamera* cam );
    virtual void setTexture( Image< Rgb< byte > >& t, SE3<> vp );
    void setTexture( Image< Rgb< byte > >* t, SE3<> vp );
    Image< Rgb< byte > >& getTexture();
    Image< Rgb< byte > >* getTextureSource();
    SE3<>& getTextureViewpoint();
    Vector<3> getFaceCentre();
    virtual Vector<3> getFaceNormal();
    bool hasFlippedNormal();
    bool operator()( PolyFace* a, PolyFace* b ) const;
protected:
    Point* p1;
    Point* p2;
    Point* p3;
    SE3<> textureViewpoint;
    Image< Rgb< byte > > texture;
    Image< Rgb< byte > >* textureSource;
    double scale;
    double offset;
    bool flipNormal;
};

#endif	/* _POLYFACE_H */

