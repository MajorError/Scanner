
#include <LinearMath/btVector3.h>

#include "GameFactory.h"
#include "../Point.h"
#include "../Edge.h"
#include "gvars3/gvars3.h"
#include "GameObject.h"
#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"

using namespace GVars3;

GameFactory::GameFactory() {
}

GameFactory::~GameFactory() {
}

WorldMap* GameFactory::create( Environment* env ) {
    WorldMap* m = new WorldMap;

    double maxGradient = GV3::get<double>( "maxGradient", 1.0 );

    set<Point*> onEdge;
    // Populate the sets of points which mark the edge of the world,
    //    and thus should be ignored (the edge isn't safe)
    for( list<Edge*>::iterator e = env->getEdges().begin(); e != env->getEdges().end(); e++ ) {
        int count = 0;
        for( set<PolyFace*>::iterator face = env->getFaces().begin(); face != env->getFaces().end(); face++ ) {
            if ( ((*e)->getStart() == (*face)->getP1() && (*e)->getEnd() == (*face)->getP2()) ||
                 ((*e)->getEnd() == (*face)->getP1() && (*e)->getStart() == (*face)->getP2()) ||
                 ((*e)->getStart() == (*face)->getP3() && (*e)->getEnd() == (*face)->getP2()) ||
                 ((*e)->getEnd() == (*face)->getP3() && (*e)->getStart() == (*face)->getP2()) ||
                 ((*e)->getStart() == (*face)->getP1() && (*e)->getEnd() == (*face)->getP3()) ||
                 ((*e)->getEnd() == (*face)->getP1() && (*e)->getStart() == (*face)->getP3()) )
                count++;
            if ( count > 1 )
                break;
        }
        if ( count <= 1 ) {
            onEdge.insert( (*e)->getStart() );
            onEdge.insert( (*e)->getEnd() );
        }
    }

    map<Point*,Waypoint*> wps;
    // Populate Waypoints in the WorldMap
    for( list<Point*>::iterator curr = env->getPoints().begin(); curr != env->getPoints().end(); curr++ ) {
        Waypoint* w = new Waypoint;
        if ( onEdge.count( *curr ) ) { // Move edge points in by 10%
            Vector<3> pos = makeVector( 0, 0, 0 );
            for( list<Edge*>::iterator e = (*curr)->getEdges().begin(); e != (*curr)->getEdges().end(); e++ ) {
                pos += ((*e)->getStart() == *curr ? (*e)->getEnd() : (*e)->getStart())->getPosition();
            }
            pos *= 0.1 / (*curr)->getEdges().size();
            w->x = (*curr)->getPosition()[0] * 0.9 + pos[0];
            w->y = (*curr)->getPosition()[1] * 0.9 + pos[1];
            w->z = (*curr)->getPosition()[2] * 0.9 + pos[2];
        } else {
            w->x = (*curr)->getPosition()[0];
            w->y = (*curr)->getPosition()[1];
            w->z = (*curr)->getPosition()[2];
        }
        wps[*curr] = w;
        m->addWaypoint( w );
    }

    // Connect these Waypoints per the geometry in the map
    for( list<Edge*>::iterator curr = env->getEdges().begin(); curr != env->getEdges().end(); curr++ ) {
        if ( onEdge.count( (*curr)->getStart() ) == 0 && onEdge.count( (*curr)->getEnd() ) == 0 )
            link( m, maxGradient, wps[(*curr)->getStart()], wps[(*curr)->getEnd()] );
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
        link( m, maxGradient, w, wps[(*curr)->getP1()] );
        link( m, maxGradient, w, wps[(*curr)->getP2()] );
        link( m, maxGradient, w, wps[(*curr)->getP3()] );
    }

    for( set<PolyFace*>::iterator curr = env->getFaces().begin(); curr != env->getFaces().end(); curr++ ) {
        if ( fwps.count( *curr ) > 0 ) {
            Waypoint* w = fwps[*curr];
            set<PolyFace*> plane;
            env->findPlanarFaces( *curr, GV3::get<double>( "planeTolerance", 0.1 ), plane );
            for( set<PolyFace*>::iterator boundary = plane.begin(); boundary != plane.end(); boundary++ ) {
                if ( (*boundary) == (*curr) )
                    continue;
                link( m, maxGradient, w, fwps[*boundary] );
                link( m, maxGradient, w, wps[(*boundary)->getP1()] );
                link( m, maxGradient, w, wps[(*boundary)->getP2()] );
                link( m, maxGradient,  w, wps[(*boundary)->getP3()] );
                fwps.erase( *boundary ); // To prevent double-traversal
            }
        }
    }

    // Remove non-navigable waypoints from the map
    m->tidyWaypoints();

    return m;
};

inline void GameFactory::link( WorldMap* m, double maxGradient, Waypoint* w1, Waypoint* w2 ) {
    double x = w1->x - w2->x;
    double y = w1->y - w2->y;
    double z = w1->z - w2->z;
    double gradX = (z*z) / (x*x);
    double gradY = (z*z) / (y*y);
    cerr << y << ": " << gradX << ", " << gradY << endl;
    if ( w1->z > w2->z )
        m->setTraversable( w1, w2, abs( gradX ) < maxGradient && abs( gradY ) < maxGradient );
    else
        m->setTraversable( w2, w1, abs( gradX ) < maxGradient && abs( gradY ) < maxGradient );
};

void GameFactory::setupCollisionPlanes( Environment* env, btDiscreteDynamicsWorld* world ) {
    btVector3 aabbMin( -10, -10, -10 );
    btVector3 aabbMax( 10, 10, 10 );

    // Place triangles on each of the polyfaces for regular colliders
    btTriangleMesh* terrainMesh = new btTriangleMesh;
    double scale = GV3::get<double>( "physicsScale", 1000 );
    aabbMin.setX( scale * aabbMin.getX() );
    aabbMin.setY( scale * aabbMin.getY() );
    aabbMin.setZ( scale * aabbMin.getZ() );
    aabbMax.setX( scale * aabbMax.getX() );
    aabbMax.setY( scale * aabbMax.getY() );
    aabbMax.setZ( scale * aabbMax.getZ() );
    for( set<PolyFace*>::iterator curr = env->getFaces().begin(); curr != env->getFaces().end(); curr++ ) {
        Vector<3> p = scale * (*curr)->getP1()->getPosition();
        btVector3 v1( p[0], p[1], p[2] );
        p = scale * (*curr)->getP2()->getPosition();
        btVector3 v2( p[0], p[1], p[2] );
        p = scale * (*curr)->getP3()->getPosition();
        btVector3 v3( p[0], p[1], p[2] );
        // Expand AABB bounds
        if ( (*curr)->hasFlippedNormal() )
            terrainMesh->addTriangle( v1, v3, v2 );
        else
            terrainMesh->addTriangle( v1, v2, v3 );
    }
    btBvhTriangleMeshShape* terrainShape = new btBvhTriangleMeshShape( terrainMesh, true, aabbMin, aabbMax );
    terrainShape->recalcLocalAabb();
    // Now that we have a shape, construct rigid body dynamics
    btDefaultMotionState* terrainMotionState = new btDefaultMotionState( btTransform( btQuaternion( 0, 0, 0, 1 ), btVector3( 0, 0, 0 ) ) );
    btRigidBody::btRigidBodyConstructionInfo terrainRigidBodyCI( 0, terrainMotionState, terrainShape );
    btRigidBody* terrainRigidBody = new btRigidBody( terrainRigidBodyCI );
    terrainRigidBody->setUserPointer( new GameObject ); // "Null-Object"
    world->addRigidBody( terrainRigidBody );
};
