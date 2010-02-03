
#ifndef _ASTARSEARCH_H
#define	_ASTARSEARCH_H

#include <list>
#include <set>
#include <map>
#include <limits>
#include "WorldMap.h"

struct AStarElement {
    Waypoint* parent;
    double distanceFromStart;
    double distanceToTarget;
};

class AStarSearch {
public:
    AStarSearch( WorldMap &m );
    virtual ~AStarSearch();
    list<Waypoint*> findPath( Waypoint* from, Waypoint* to );
private:
    WorldMap &wm;
};

#endif	/* _ASTARSEARCH_H */

