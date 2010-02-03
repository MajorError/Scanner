
#ifndef _WORLDMAP_H
#define	_WORLDMAP_H

#include <list>

using namespace std;

struct Waypoint {
    double x, y, z;
    list<Waypoint*> traversable;
    list<double> distances;

    inline double operator-( Waypoint* to ) {
        return (x - to->x) * (x - to->x)
                 + (y - to->y) * (y - to->y)
                 + (z - to->z) * (z - to->z);
    }
};

class WorldMap {
public:
    WorldMap() {};
    virtual ~WorldMap() {};

    void addWaypoint( Waypoint* w );
    void setTraversable( Waypoint* from, Waypoint* to );
    void tidyWaypoints();
    static bool toTidy( const Waypoint* p );
private:
    list<Waypoint*> waypoints;
};

#endif	/* _WORLDMAP_H */

