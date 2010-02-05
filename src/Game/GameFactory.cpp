
#include "GameFactory.h"
#include "../Point.h"
#include "../Edge.h"
#include "gvars3/gvars3.h"

using namespace GVars3;

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

    map<PolyFace*,Waypoint*> fwps;
    for( set<PolyFace*>::iterator curr = env->getFaces().begin(); curr != env->getFaces().end(); curr++ ) {
        Waypoint* w = new Waypoint;
        w->x = (*curr)->getFaceCentre()[0];
        w->y = (*curr)->getFaceCentre()[1];
        w->z = (*curr)->getFaceCentre()[2];
        m->addWaypoint( w );
        fwps[*curr] = w;

        // Can always traverse from centre to corner of face
        m->setTraversable( w, wps[(*curr)->getP1()] );
        m->setTraversable( w, wps[(*curr)->getP2()] );
        m->setTraversable( w, wps[(*curr)->getP3()] );
    }

    for( set<PolyFace*>::iterator curr = env->getFaces().begin(); curr != env->getFaces().end(); curr++ ) {
        if ( fwps.count( *curr ) > 0 ) {
            Waypoint* w = fwps[*curr];
            set<PolyFace*> plane;
            env->findPlanarFaces( *curr, GV3::get<double>( "planeTolerance", 0.1 ), plane );
            for( set<PolyFace*>::iterator boundary = plane.begin(); boundary != plane.end(); boundary++ ) {
                if ( (*boundary) == (*curr) )
                    continue;
                m->setTraversable( w, fwps[*boundary] );
                m->setTraversable( w, wps[(*boundary)->getP1()] );
                m->setTraversable( w, wps[(*boundary)->getP2()] );
                m->setTraversable( w, wps[(*boundary)->getP3()] );
                fwps.erase( *boundary ); // To prevent double-traversal
            }
        }
    }
    
    return m;
};
