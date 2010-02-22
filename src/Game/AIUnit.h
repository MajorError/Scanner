
#ifndef _AIUNIT_H
#define	_AIUNIT_H

#include "AStarSearch.h"
#include "WorldMap.h"
#include <TooN/TooN.h>

using namespace TooN;

class AIUnit {
public:
    AIUnit( WorldMap* m, btDynamicsWorld* w, double x, double y, double z );
    virtual ~AIUnit();
    void tick();
    double getX();
    double getY();
    double getZ();
    double getRotationAngle();
    Vector<3> getRotationAxis();
    void navigateTo( Waypoint* goal );
    void push( double x, double y, double z );
    void removeFromWorld( btDynamicsWorld* w );

    static int type;
private:
    int currTick, lastNode;
    double xDir, yDir, zDir;
    double xPos, yPos, zPos;
    double rotAngle;
    Vector<3> rotAxis;
    btRigidBody* boxBody;
    list<Waypoint*> path;
    WorldMap* map;
    AStarSearch search;
    static btCollisionShape* boxShape;
};

#endif	/* _AIUNIT_H */

