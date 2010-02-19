
#include "WorldMap.h"
#include <limits>
#include <iostream>
#include <vector>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>


WorldMap::WorldMap() {
};

WorldMap::~WorldMap() {
};

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

Waypoint* WorldMap::findNearest( double x, double y, double z ) {
    Waypoint* tmp = new Waypoint;
    tmp->x = x;
    tmp->y = y;
    tmp->z = z;
    Waypoint* nearest = findNearest( tmp );
    delete tmp;
    return nearest;
};

Waypoint* WorldMap::findNearest( Waypoint* p ) {
    double bestDistance = numeric_limits<double>::max();
    Waypoint* best = NULL;
    for( list<Waypoint*>::iterator curr = waypoints.begin(); curr != waypoints.end(); curr++ ) {
        if ( (*curr) == p )
            return p;
        double d = (*p) - (*curr);
        if ( d < bestDistance ) {
            best = *curr;
            bestDistance = d;
        }
    }
    return best;
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
};

list<Waypoint*>& WorldMap::getWaypoints() {
    return waypoints;
};