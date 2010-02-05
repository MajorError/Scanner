
#ifndef _AIUNIT_H
#define	_AIUNIT_H

#include "AStarSearch.h"
#include "WorldMap.h"

class AIUnit {
public:
    AIUnit( WorldMap* m );
    AIUnit( WorldMap* m, double x, double y, double z );
    virtual ~AIUnit();
    void tick();
    double getX();
    double getY();
    double getZ();
    void navigateTo( Waypoint* goal );
private:
    double xDir, yDir, zDir, velocity;
    double xPos, yPos, zPos;
    list<Waypoint*> path;
    WorldMap* map;
    AStarSearch search;
};

#endif	/* _AIUNIT_H */

