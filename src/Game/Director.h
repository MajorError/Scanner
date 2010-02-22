
#ifndef _DIRECTOR_H
#define	_DIRECTOR_H

#include "WorldMap.h"
#include "AIUnit.h"
#include "Projectile.h"
#include <TooN/TooN.h>

using namespace TooN;

class Director {
public:
    Director( btDynamicsWorld* w, WorldMap* m );
    virtual ~Director();
    void tick();
    vector<AIUnit*>& getUnits();
    AIUnit* addAI( btDynamicsWorld* w, double x, double y, double z );
    vector<Projectile*>& getProjectiles();
    Projectile* addProjectile( btDynamicsWorld* w, double x, double y, double z );
    void registerImpact();
    void registerDeath();
    void registerAIWin();
    int getScore();
private:
    btDynamicsWorld* dynamicsWorld;
    WorldMap* map;
    Vector<3> lowerBound, upperBound;
    vector<Projectile*> projectiles;
    vector<AIUnit*> aiUnits;
    int currTick;
    int lastSpawn;
    vector<Waypoint*> startPoints, endPoints;
    int score;
};

#endif	/* _DIRECTOR_H */

