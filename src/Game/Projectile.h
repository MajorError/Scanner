
#ifndef _PROJECTILE_H
#define	_PROJECTILE_H

#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>

class Projectile {
public:
    Projectile( btDynamicsWorld* w, double x, double y, double z );
    virtual ~Projectile();
    void tick();
    double getX();
    double getY();
    double getZ();
    void push( double x, double y, double z );
private:
    double xPos, yPos, zPos;
    btRigidBody* ballBody;
};

#endif	/* _PROJECTILE_H */

