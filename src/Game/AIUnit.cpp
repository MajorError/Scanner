#include "AIUnit.h"
#include <gvars3/gvars3.h>
#include <cmath>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
using namespace GVars3;

btCollisionShape* AIUnit::boxShape;

AIUnit::AIUnit( WorldMap* m, btDynamicsWorld* w, double x, double y, double z )
: velocity( 0 ), xPos( x ), yPos( y ), zPos( z ), map( m ), search( m ) {
    btVector3 inertia( 1, 1, 1 );
    if ( AIUnit::boxShape == NULL ) {
        double ds = GV3::get<double>( "ptSize", 0.05 );
        AIUnit::boxShape = new btBoxShape( btVector3( ds, ds, ds ) );
        AIUnit::boxShape->calculateLocalInertia( GV3::get<double>( "aiMass", 0.5 ), inertia );
    }
    btDefaultMotionState* boxMotionState = new btDefaultMotionState( btTransform( btQuaternion( 0, 0, 0, 1 ), btVector3( x, y, z ) ) );
    btRigidBody::btRigidBodyConstructionInfo boxRigidBodyCI( GV3::get<double>( "aiMass" ), boxMotionState, boxShape, inertia );
    
    boxBody = new btRigidBody( boxRigidBodyCI );
    w->addRigidBody( boxBody );
};

AIUnit::~AIUnit() {
};

#define ABSDIFF( a, b ) (a > b ? a - b : b - a)
void AIUnit::tick() {

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

    velocity = GV3::get<double>( "aiSpeed", 0.001 );
    double tolerance = GV3::get<double>( "aiTolerance", GV3::get<double>( "ptSize" ) );
    //cerr << "tock @ " << xPos << ", " << yPos << ", " << zPos << endl;
    if ( ABSDIFF( path.front()->x, xPos ) < tolerance
            && ABSDIFF( path.front()->y, yPos ) < tolerance
            && ABSDIFF( path.front()->z, zPos ) < tolerance ) {
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
    double div = (abs( xDir ) + abs( yDir ) + abs( zDir )) / velocity ;
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
    cerr << "Got path of " << path.size() << " points" << endl;
};

void AIUnit::push( double x, double y, double z ) {
    boxBody->activate( true );
    boxBody->applyCentralForce( btVector3( x, y, z ) );
};