
#include "GameFactory.h"
#include "../Point.h"
#include "../Edge.h"

GameFactory::GameFactory() {
}

GameFactory::~GameFactory() {
}

WorldMap* GameFactory::create( Environment* env ) {
    WorldMap* m = new WorldMap;

    map<Point*,Waypoint*> wps;
    // Populate Waypoints in the WorldMap
    for( list<Point*>::iterator curr = env->getPoints().begin(); curr != env->getPoints().end(); curr++ ) {
        Waypoint* w = new Waypoint;
        w->x = (*curr)->getPosition()[0];
        w->y = (*curr)->getPosition()[1];
        w->z = (*curr)->getPosition()[2];
        wps[*curr] = w;
        m->addWaypoint( w );
    }

    // Connect these Waypoints per the geometry in the map
    for( list<Edge*>::iterator curr = env->getEdges().begin(); curr != env->getEdges().end(); curr++ ) {
        m->setTraversable( wps[(*curr)->getStart()], wps[(*curr)->getEnd()] );
    }
    
    return m;
};
