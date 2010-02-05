
#include "AIUnit.h"
#include <gvars3/gvars3.h>
#include <cmath>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
using namespace GVars3;

AIUnit::AIUnit( WorldMap* m, btDynamicsWorld* w, double x, double y, double z )
: velocity( 0 ), xPos( x ), yPos( y ), zPos( z ), map( m ), search( m ) {
    btCollisionShape* ballShape = new btSphereShape( 0.05 );
    btDefaultMotionState* ballMotionState = new btDefaultMotionState( btTransform( btQuaternion( 0, 0, 0, 1 ), btVector3( x, y, z ) ) );
    btVector3 inertia( 0, 0, 0 );
    ballShape->calculateLocalInertia( GV3::get<double>( "aiMass", 0.5 ), inertia );
    btRigidBody::btRigidBodyConstructionInfo ballRigidBodyCI( GV3::get<double>( "aiMass" ), ballMotionState, ballShape, inertia );
    
    ballBody = new btRigidBody( ballRigidBodyCI );
    w->addRigidBody( ballBody );
}

AIUnit::~AIUnit() {
}

#define ABSDIFF( a, b ) (a > b ? a - b : b - a)
void AIUnit::tick() {

    btTransform trans;
    ballBody->getMotionState()->getWorldTransform( trans );
    xPos = trans.getOrigin().getX();
    yPos = trans.getOrigin().getY();
    zPos = trans.getOrigin().getZ();


    /* Disabled for now. TODO: How to integrate this with Bullet?
    if ( path.size() < 1 )
        return;

    velocity = GV3::get<double>( "aiSpeed", 0.01 );
    //cerr << "tock @ " << xPos << ", " << yPos << ", " << zPos << endl;
    if ( ABSDIFF( path.front()->x, xPos ) < velocity
            && ABSDIFF( path.front()->y, yPos ) < velocity
            && ABSDIFF( path.front()->z, zPos ) < velocity ) {
        cerr << "\tPath Point Reached: ";
        // Test if this is a goal object to be deleted (i.e. not in the node graph)
        if ( path.front()->traversable.size() == 0 )
            delete path.front();
        path.pop_front();
        // Check if we've reached our goal
        if ( path.size() == 0 ) {
            cerr << "STOP" << endl;
            velocity = 0;
            return;
        }
        cerr << "Next stop = " << path.front()->x << ", " << path.front()->y << ", " << path.front()->z << " -> " << endl;
    }
    // Set up new {x,y,z}Dir
    xDir = path.front()->x - xPos;
    yDir = path.front()->y - yPos;
    zDir = path.front()->z - zPos;
    // Normalise the vector
    double div = abs( xDir ) + abs( yDir ) + abs( zDir );
    xDir /= div;
    yDir /= div;
    zDir /= div;
    xPos += velocity * xDir;
    yPos += velocity * yDir;
    zPos += velocity * zDir;*/
};

double AIUnit::getX() {
    return xPos;
};

double AIUnit::getY() {
    return yPos;
};

double AIUnit::getZ() {
    return zPos;
};

void AIUnit::navigateTo( Waypoint* goal ) {
    Waypoint* from = new Waypoint;
    from->x = xPos;
    from->y = yPos;
    from->z = zPos;
    path = search.findPath( from, goal );
    path.push_front( from );
    if( path.back()->x != goal->x || path.back()->y != goal->y || path.back()->z != goal->z )
        path.push_back( goal );
    cerr << "Got path of " << path.size() << " points" << endl;
};