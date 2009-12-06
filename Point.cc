
#include <vector>

#include "Point.h"
using namespace std;

void Point::addEdge( Edge* e ) {
    edges.push_back( e );
};

vector<Edge*>* Point::getEdges() {
    return &edges;
};

Vector<3>* Point::getPosition(){
    return &position;
};

void Point::setPosition( double x, double y, double z ){
    position = makeVector( x, y, z );
};

void Point::setPosition( Vector<3> pos ){
    position = pos;
};