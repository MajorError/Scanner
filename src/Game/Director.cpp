
#include "Director.h"
#include <gvars3/gvars3.h>
#include <limits>

using namespace GVars3;

Director::Director( btDynamicsWorld* w, WorldMap* m ) : dynamicsWorld( w ), map( m ), currTick( 0 ), lastSpawn( 0 ) {
};

Director::~Director() {
};

vector<AIUnit*>& Director::getUnits() {
    return aiUnits;
};

AIUnit* Director::addAI( btDynamicsWorld* w, double x, double y, double z ) {
    AIUnit* a = new AIUnit( map, w, x, y, z );
    aiUnits.push_back( a );
    return a;
};

vector<Projectile*>& Director::getProjectiles() {
    return projectiles;
};

Projectile* Director::addProjectile( btDynamicsWorld* w, double x, double y, double z ) {
    Projectile* a = new Projectile( w, x, y, z );
    projectiles.push_back( a );
    return a;
};

void Director::tick() {
    currTick++;
    for( vector<AIUnit*>::iterator curr = aiUnits.begin(); curr != aiUnits.end(); curr++ ) {
        (*curr)->tick();
    }
    for( vector<Projectile*>::iterator curr = projectiles.begin(); curr != projectiles.end(); curr++ ) {
        (*curr)->tick();
    }
    
    /*if ( currTick - lastSpawn > GV3::get<int>( "spawnFreq", 50 )
            && ((double)rand() / (double)RAND_MAX) < GV3::get<double>( "spawnProb", 0.1 ) ) {
        // Spawn a new character
        Waypoint* target = NULL;
        int wpID = ((double)rand() / (double)RAND_MAX) * map->getWaypoints().size();
        for( list<Waypoint*>::iterator curr = map->getWaypoints().begin(); curr != map->getWaypoints().end() && wpID >= 0; curr++, wpID-- ) {
            target = *curr;
        }
        if ( target != NULL ) {
            cerr << "POP @ " << target->x << ", " << target->y << ", " << target->z << endl;
            addAI( dynamicsWorld, target->x, target->y, target->z+1 );
        }
    }*/
};

