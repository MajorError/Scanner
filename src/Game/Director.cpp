
#include "Director.h"
#include <gvars3/gvars3.h>
#include <limits>
#include <cmath>

using namespace GVars3;

Director::Director( btDynamicsWorld* w, WorldMap* m ) : dynamicsWorld( w ), map( m ), currTick( 0 ), lastSpawn( 0 ), score( 0 ) {
    // Find leftmost and rightmost points;
    Waypoint* leftMost = NULL;
    Waypoint* rightMost = NULL;
    for( list<Waypoint*>::iterator curr = m->getWaypoints().begin(); curr != m->getWaypoints().end(); curr++ ) {
        if ( leftMost == NULL || leftMost->x > (*curr)->x ) {
            leftMost = *curr;
        }
        if ( rightMost == NULL || rightMost->x < (*curr)->x ) {
            rightMost = *curr;
        }
    }
    double tol = GV3::get<double>( "spawnTolerance", 0.3 );
    for( list<Waypoint*>::iterator curr = m->getWaypoints().begin(); curr != m->getWaypoints().end(); curr++ ) {
        // If a point could be both start and end, default to end point (more
        //   variety in paths the AIs take)
        if( abs( (*curr)->x - rightMost->x ) < tol ) {
            cerr << ">> End @ " << (*curr)->x << endl;
            endPoints.push_back( *curr );
        } else if( abs( (*curr)->x - leftMost->x ) < tol ) {
            cerr << ">> Start @ " << (*curr)->x << endl;
            startPoints.push_back( *curr );
        }
    }
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


void Director::registerImpact() {
    score += GV3::get<int>( "game.impactScore", 0 );
};

void Director::registerDeath() {
    score += GV3::get<int>( "game.deathScore", 100 );
};

void Director::registerAIWin() {
    score += GV3::get<int>( "game.goalScore", -300 );
};

int Director::getScore() {
    return score;
};

void Director::tick() {
    currTick++;
    for( vector<AIUnit*>::iterator curr = aiUnits.begin(); curr != aiUnits.end(); curr++ )
        (*curr)->tick( this );
    for( vector<Projectile*>::iterator curr = projectiles.begin(); curr != projectiles.end(); curr++ )
        (*curr)->tick();
    
    if ( currTick - lastSpawn > GV3::get<int>( "spawnFreq", 3000 )
            && ((double)rand() / (double)RAND_MAX) < GV3::get<double>( "spawnProb", 0.0001 ) ) {
        lastSpawn = currTick;
        // Spawn a new character
        Waypoint* target = NULL;
        int wpID = ((double)rand() / (double)RAND_MAX) * startPoints.size();
        for( vector<Waypoint*>::iterator curr = startPoints.begin(); curr != startPoints.end() && wpID >= 0; curr++, wpID-- )
            target = *curr;
        if ( target != NULL ) {
            double x = target->x;
            double y = target->y;
            double z = target->z+0.5;
            AIUnit* a = addAI( dynamicsWorld, x, y, z );
            // Choose a goal point
            target = NULL;
            wpID = ((double)rand() / (double)RAND_MAX) * endPoints.size();
            for( vector<Waypoint*>::iterator curr = endPoints.begin(); curr != endPoints.end() && wpID >= 0; curr++, wpID-- )
                target = *curr;
            if ( target != NULL )
                a->navigateTo( target );
        }
    }
};

