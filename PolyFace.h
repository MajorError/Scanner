/* 
 * File:   PolyFace.h
 * Author: majorerror
 *
 * Created on 08 December 2009, 18:16
 */

#ifndef _POLYFACE_H
#define	_POLYFACE_H

#include "Edge.h"


class PolyFace {
public:
    PolyFace() : p1( NULL ), p2( NULL ), p3( NULL ) {};
    PolyFace( Point* a, Point* b, Point* c );
    virtual ~PolyFace() {};
    Point* getP1();
    Point* getP2();
    Point* getP3();
    bool operator()( PolyFace* a, PolyFace* b ) const;
protected:
    Point* p1;
    Point* p2;
    Point* p3;

};

#endif	/* _POLYFACE_H */

