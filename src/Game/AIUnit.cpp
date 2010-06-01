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
: currTick( 0 ), lastNode( 0 ), xPos( x ), yPos( y ), zPos( z ), xPosPrev( x ),
        yPosPrev( y ), zPosPrev( z ), map( m ), search( m ) {
    btVector3 inertia( 1, 1, 1 );
    double ds = GV3::get<double>( "ptSize", 0.03 );
    double scale = GV3::get<double>( "physicsScale" );
    if ( AIUnit::boxShape == NULL ) {
        AIUnit::boxShape = new btBoxShape( btVector3( scale * ds, scale * ds, scale * ds ) );
        AIUnit::boxShape->calculateLocalInertia( GV3::get<double>( "aiMass", 0.5 ), inertia );
    }
    btDefaultMotionState* boxMotionState = new btDefaultMotionState( btTransform( btQuaternion( 0, 0, 0, 1 ), scale * btVector3( x, y, z ) ) );
    btRigidBody::btRigidBodyConstructionInfo boxRigidBodyCI( GV3::get<double>( "aiMass" ), boxMotionState, boxShape, inertia );
    
    boxBody = new btRigidBody( boxRigidBodyCI );
    boxBody->setUserPointer( this );
    boxBody->setCcdMotionThreshold( scale * ds );
    boxBody->setCcdSweptSphereRadius( 0.2f * scale * ds );
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

    btVector3 pos = boxBody->getWorldTransform().getOrigin() / GV3::get<double>( "physicsScale" );
    xPos = pos.getX();
    yPos = pos.getY();
    zPos = pos.getZ();
    btTransform t;
    boxBody->getMotionState()->getWorldTransform( t );
    btQuaternion q = t.getRotation();
    rotAngle = q.getAngle();
    btVector3 axis = q.getAxis();
    rotAxis[0] = axis.getX();
    rotAxis[1] = axis.getY();
    rotAxis[2] = axis.getZ();


    if ( path.size() < 1 )
        return;

    double tolerance = GV3::get<double>( "aiTolerance", GV3::get<double>( "ptSize" ) );
    // Just need to be vertically above a point for a "hit"
    if ( ABSDIFF( path.front()->x, xPos ) < tolerance
            && ABSDIFF( path.front()->y, yPos ) < tolerance ) {
        cerr << "HIT (" << path.front()->x << ", " << path.front()->y << ", " << path.front()->z << ")" << endl;
        lastNode = currTick;
        // Test if this is a goal object to be deleted (i.e. not in the node graph)
        if ( path.front()->traversable.size() == 0 )
            delete path.front();
        path.pop_front();
        // Check if we've reached our goal
        if ( path.size() == 0 ) {
            cerr << "AI reached goal" << endl;
            // Drop off the planet, so that we get culled
            btTransform death;
            death.getOrigin().setZ( -50000 );
            boxBody->setWorldTransform( death );
            d->registerAIWin();
            return;
        }
    }
    // Replan if we haven't seen a node recently
    if ( currTick - lastNode > GV3::get<int>( "aiPatience", 100000 ) )
        navigateTo( path.back() );

    tolerance /= GV3::get<double>( "physicsScale" );
    if ( ABSDIFF( xPosPrev, xPos ) < tolerance && ABSDIFF( yPosPrev, yPos ) < tolerance
        && ABSDIFF( zPosPrev, zPos ) < tolerance /*&& currTick % GV3::get<int>( "aiThrustFreq", 3000 ) == 0*/ ) {
        // Set up new {x,y,z}Dir
        xDir = path.front()->x - xPos;
        yDir = path.front()->y - yPos;
        zDir = 1.0; // Constant slight "up hop" when applying force
        
        // Normalise the (x,y) vector of movement, apply velocity
        double div = (abs( xDir ) + abs( yDir )) / GV3::get<double>( "aiSpeed", 20 ) ;
        xDir /= div;
        yDir /= div;

        boxBody->clearForces();
        cerr << ">> At (" << ABSDIFF( path.front()->x, xPos ) << ", " << ABSDIFF( path.front()->y, yPos )
             << ", " << ABSDIFF( path.front()->z, zPos ) << ") " << "{" << abs( boxBody->getAngularVelocity().getZ() ) << "}  "
             << "apply force " << xDir << ", " << yDir << ", " << zDir << endl;
        boxBody->applyCentralImpulse( btVector3( xDir, yDir, zDir ) );
        boxBody->activate( true );
    }
    xPosPrev = xPos;
    yPosPrev = yPos;
    zPosPrev = zPos;
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
    cerr << "Got Path from (" << xPos << "," << yPos << "," << zPos << "): ";
    for( list<Waypoint*>::iterator pt = path.begin(); pt != path.end(); pt++ ) {
        cerr << "(" << (*pt)->x << "," << (*pt)->y << "," << (*pt)->z << ")  ";
    }
    cerr << endl;
};

void AIUnit::push( double x, double y, double z ) {
    boxBody->activate( true );
    boxBody->applyCentralForce( btVector3( x, y, z ) );
};