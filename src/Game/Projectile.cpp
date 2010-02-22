
#include "Projectile.h"
#include <gvars3/gvars3.h>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <LinearMath/btDefaultMotionState.h>

using namespace GVars3;

btCollisionShape* Projectile::ballShape = NULL;

Projectile::Projectile( btDynamicsWorld* w, double x, double y, double z )
: xPos( x ), yPos( y ), zPos( z ) {
    if ( Projectile::ballShape == NULL ) {
        Projectile::ballShape = new btSphereShape( GV3::get<double>( "ptSize", 0.05 ) / 2 );
        btVector3 inertia( 0, 0, 0 );
        Projectile::ballShape->calculateLocalInertia( GV3::get<double>( "projectileMass", 0.4 ), inertia );
    }
    btDefaultMotionState* ballMotionState = new btDefaultMotionState( btTransform( btQuaternion( 0, 0, 0, 1 ), btVector3( x, y, z ) ) );
    btRigidBody::btRigidBodyConstructionInfo ballRigidBodyCI( GV3::get<double>( "projectileMass" ), ballMotionState, Projectile::ballShape );
    ballBody = new btRigidBody( ballRigidBodyCI );
    w->addRigidBody( ballBody );
}

Projectile::~Projectile() {
};

void Projectile::removeFromWorld( btDynamicsWorld* w ) {
    w->removeRigidBody( ballBody );
    delete ballBody;
};

void Projectile::tick() {
    btTransform trans;
    ballBody->getMotionState()->getWorldTransform( trans );
    xPos = trans.getOrigin().getX();
    yPos = trans.getOrigin().getY();
    zPos = trans.getOrigin().getZ();
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
    ballBody->applyCentralForce( btVector3( x, y, z ) );
};