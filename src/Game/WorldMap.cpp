
#include "WorldMap.h"
#include <iostream>

void WorldMap::addWaypoint( Waypoint* w ) {
    waypoints.push_back( w );
};

void WorldMap::setTraversable( Waypoint* from, Waypoint* to ) {
    double d = (*from) - to;
    from->traversable.push_back( to );
    from->distances.push_back( d );
    to->traversable.push_back( from );
    to->distances.push_back( d );
};

void WorldMap::tidyWaypoints() {
    waypoints.remove_if( WorldMap::toTidy );
};

bool WorldMap::toTidy( const Waypoint* p ) {
    if ( p->traversable.size() > 0 )
        return false;
    cerr << "Delete " << p << endl;
    delete p;
    return true;
}