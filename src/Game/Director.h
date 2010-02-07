
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
private:
    vector<AIUnit*> aiUnits;
    vector<Projectile*> projectiles;
    btDynamicsWorld* dynamicsWorld;
    WorldMap* map;
    Vector<3> lowerBound, upperBound;
    int currTick;
    int lastSpawn;
};

#endif	/* _DIRECTOR_H */

