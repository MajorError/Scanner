#include "AIUnit.h"
#include "Director.h"
#include <gvars3/gvars3.h>
#include <cmath>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>

using namespace GVars3;

int AIUnit::type = (1 << 1);
btCollisionShape* AIUnit::boxShape;

AIUnit::AIUnit( WorldMap* m, btDynamicsWorld* w, double x, double y, double z )
: currTick( 0 ), lastNode( 0 ), xPos( x ), yPos( y ), zPos( z ), map( m ), search( m ) {
    btVector3 inertia( 1, 1, 1 );
    double ds = GV3::get<double>( "ptSize", 0.05 );
    if ( AIUnit::boxShape == NULL ) {
        AIUnit::boxShape = new btBoxShape( btVector3( ds, ds, ds ) );
        AIUnit::boxShape->calculateLocalInertia( GV3::get<double>( "aiMass", 0.5 ), inertia );
    }
    btDefaultMotionState* boxMotionState = new btDefaultMotionState( btTransform( btQuaternion( 0, 0, 0, 1 ), btVector3( x, y, z ) ) );
    btRigidBody::btRigidBodyConstructionInfo boxRigidBodyCI( GV3::get<double>( "aiMass" ), boxMotionState, boxShape, inertia );
    
    boxBody = new btRigidBody( boxRigidBodyCI );
    boxBody->setUserPointer( this );
    boxBody->setCcdMotionThreshold( ds );
    boxBody->setCcdSweptSphereRadius( 0.2f * ds );
    w->addRigidBody( boxBody );
};

AIUnit::~AIUnit() {
};

void AIUnit::removeFromWorld( btDynamicsWorld* w ) {
    w->removeRigidBody( boxBody );
    delete boxBody;
};

int AIUnit::getType() {
    return AIUnit::type;
};

#define ABSDIFF( a, b ) (a > b ? a - b : b - a)
void AIUnit::tick( Director* d ) {

    currTick++;

    btTransform trans = boxBody->getWorldTransform();
    xPos = trans.getOrigin().getX();
    yPos = trans.getOrigin().getY();
    zPos = trans.getOrigin().getZ();
    btQuaternion q = trans.getRotation();
    rotAngle = q.getAngle();
    btVector3 axis = q.getAxis();
    rotAxis[0] = axis.getX();
    rotAxis[1] = axis.getY();
    rotAxis[2] = axis.getZ();


    if ( path.size() < 1 )
        return;

    double tolerance = GV3::get<double>( "aiTolerance", GV3::get<double>( "ptSize" ) );
    //cerr << "tock @ " << xPos << ", " << yPos << ", " << zPos << endl;
    if ( ABSDIFF( path.front()->x, xPos ) < tolerance
            && ABSDIFF( path.front()->y, yPos ) < tolerance
            && ABSDIFF( path.front()->z, zPos ) < tolerance ) {
        lastNode = currTick;
        //cerr << "\tPath Point Reached: ";
        // Test if this is a goal object to be deleted (i.e. not in the node graph)
        if ( path.front()->traversable.size() == 0 )
            delete path.front();
        path.pop_front();
        // Check if we've reached our goal
        if ( path.size() == 0 ) {
            cerr << "AI reached goal" << endl;
            // Drop off the planet, so that we get culled
            btTransform death;
            death.getOrigin().setZ( -500 );
            boxBody->setWorldTransform( death );
            d->registerAIWin();
            return;
        }
        //cerr << "Next stop = " << path.front()->x << ", " << path.front()->y << ", " << path.front()->z << " -> " << endl;
    }
    // Replan if we haven't seen a node recently
    if ( currTick - lastNode > GV3::get<int>( "aiPatience", 500 ) )
        navigateTo( path.back() );
    // Set up new {x,y,z}Dir
    xDir = path.front()->x - xPos;
    yDir = path.front()->y - yPos;
    zDir = path.front()->z - zPos;
    // Normalise the vector, apply velocity
    double div = (abs( xDir ) + abs( yDir ) + abs( zDir )) / GV3::get<double>( "aiSpeed", 0.001 ) ;
    xDir /= div;
    yDir /= div;
    zDir /= div;
    boxBody->translate( btVector3( xDir, yDir, zDir ) );
    boxBody->activate( true );
    //cerr << "\t" << velocity * xDir << ", " << velocity * yDir << ", " << velocity * zDir << endl;
    /*if ( boxBody->getAngularVelocity().length() < GV3::get<double>( "aiThresholdSpeed", 10 ) )
        push( velocity * xDir, velocity * yDir, velocity * zDir );*/
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

double AIUnit::getRotationAngle() {
    return rotAngle;
};

Vector<3> AIUnit::getRotationAxis() {
    return rotAxis;
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
};

void AIUnit::push( double x, double y, double z ) {
    boxBody->activate( true );
    boxBody->applyCentralForce( btVector3( x, y, z ) );
};