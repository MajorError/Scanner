
#include "Projectile.h"
#include <gvars3/gvars3.h>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <LinearMath/btDefaultMotionState.h>

using namespace GVars3;

int Projectile::type = (1 << 2);
btCollisionShape* Projectile::ballShape = NULL;

Projectile::Projectile( btDynamicsWorld* w, double x, double y, double z )
: xPos( x ), yPos( y ), zPos( z ) {
    double ds = GV3::get<double>( "ptSize", 0.05 ) / 2;
    double scale = GV3::get<double>( "physicsScale" );
    if ( Projectile::ballShape == NULL ) {
        Projectile::ballShape = new btSphereShape( scale * ds );
        btVector3 inertia( 0, 0, 0 );
        Projectile::ballShape->calculateLocalInertia( GV3::get<double>( "projectileMass", 0.4 ), inertia );
    }
    btDefaultMotionState* ballMotionState = new btDefaultMotionState( btTransform( btQuaternion( 0, 0, 0, 1 ), scale * btVector3( x, y, z ) ) );
    btRigidBody::btRigidBodyConstructionInfo ballRigidBodyCI( GV3::get<double>( "projectileMass" ), ballMotionState, Projectile::ballShape );
    ballBody = new btRigidBody( ballRigidBodyCI );
    ballBody->setUserPointer( this );
    ballBody->setCcdMotionThreshold( scale * ds );
    ballBody->setCcdSweptSphereRadius( 0.2f * scale * ds );
    w->addRigidBody( ballBody );
};

Projectile::~Projectile() {
};

void Projectile::removeFromWorld( btDynamicsWorld* w ) {
    w->removeRigidBody( ballBody );
    delete ballBody;
};

int Projectile::getType() {
    return Projectile::type;
};

void Projectile::tick() {
    btTransform t;
    ballBody->getMotionState()->getWorldTransform( t );
    btVector3 pos = t.getOrigin() / GV3::get<double>( "physicsScale" );
    xPos = pos.getX();
    yPos = pos.getY();
    zPos = pos.getZ();
};

double Projectile::getX() {
    return xPos;
};

double Projectile::getY() {
    return yPos;
};

double Projectile::getZ() {
    return zPos;
};

void Projectile::push( double x, double y, double z ) {
    ballBody->applyCentralImpulse( btVector3( x, y, z ) );
};