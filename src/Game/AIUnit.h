
#ifndef _AIUNIT_H
#define	_AIUNIT_H

#include "AStarSearch.h"
#include "WorldMap.h"

class AIUnit {
public:
    AIUnit( WorldMap* m, btDynamicsWorld* w, double x, double y, double z );
    virtual ~AIUnit();
    void tick();
    double getX();
    double getY();
    double getZ();
    void navigateTo( Waypoint* goal );
    void push( double x, double y, double z );
private:
    double xDir, yDir, zDir, velocity;
    double xPos, yPos, zPos;
    btRigidBody* boxBody;
    list<Waypoint*> path;
    WorldMap* map;
    AStarSearch search;
};

#endif	/* _AIUNIT_H */

