
#include "WorldMap.h"

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

