/* 
 * File:   Point.h
 * Author: majorerror
 *
 * Created on 06 December 2009, 10:00
 */

#ifndef _POINT_H
#define	_POINT_H

#include <TooN/TooN.h>
#include "Edge.h"
using namespace TooN;
using namespace std;

class Point {
public:
    Point( double x, double y, double z ) : position( makeVector( x, y, z ) ){};
    Point( Vector<3> pos ) : position( pos ){};
    virtual ~Point() {};

    void addEdge( Edge* e );
    vector<Edge*>* getEdges();
    Vector<3> getPosition();
    void setPosition( double x, double y, double z );
    void setPosition( Vector<3> pos );

private:
    Vector<3> position;
    vector<Edge*> edges;
};

#endif	/* _POINT_H */

