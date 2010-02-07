
#ifndef _WORLDMAP_H
#define	_WORLDMAP_H

#include <list>
#include <vector>
#include <string>
#include <btBulletDynamicsCommon.h>

using namespace std;

class AIUnit; // forward decl

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
    WorldMap();
    virtual ~WorldMap();

    void addWaypoint( Waypoint* w );
    void setTraversable( Waypoint* from, Waypoint* to );
    void tidyWaypoints();
    static bool toTidy( const Waypoint* p );
    Waypoint* findNearest( double x, double y, double z );
    Waypoint* findNearest( Waypoint* p );
    void tickAll();
    AIUnit* addAI( btDynamicsWorld* w, double x, double y, double z );
    list<Waypoint*>& getWaypoints();
    vector<AIUnit*>& getUnits();
protected:
    list<Waypoint*> waypoints;
    vector<AIUnit*> aiUnits;
    /*double thisIsInteresting;
    double soIsthis;
    string whatAboutThis;*/
};

#endif	/* _WORLDMAP_H */

