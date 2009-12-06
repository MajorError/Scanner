/* 
 * File:   Edge.h
 * Author: majorerror
 *
 * Created on 06 December 2009, 10:05
 */

#ifndef _EDGE_H
#define	_EDGE_H

class Point; // Forward declaration

class Edge {
public:
    Edge(Point* f, Point* t) : from( f ), to( t ){};
    virtual ~Edge(){};

protected:
    Point* from;
    Point* to;
};

#endif	/* _EDGE_H */

