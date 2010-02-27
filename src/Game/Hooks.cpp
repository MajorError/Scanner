#include "../Tool.h"
#include "../VisionProcessor.h"
#include "../GUICommand.h"
#include "WorldMap.h"
#include "AIUnit.h"
#include "Director.h"
#include "GameObject.h"
#include "GLDebugDrawer.h"
#include <btBulletDynamicsCommon.h>
#include <TooN/TooN.h>

using namespace CVD;
using namespace TooN;

MK_VISION_PLUGIN( tick, btClock clock; public: static void callback( btDynamicsWorld *world, btScalar timeStep ); \
static WorldMap* map; static Director* director; static btDiscreteDynamicsWorld* dynamicsWorld;)
WorldMap* tick::map = NULL;
Director* tick::director = NULL;
btDiscreteDynamicsWorld* tick::dynamicsWorld = NULL;
void tick::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    // No map, no game! Disables self
    if ( map == NULL ) {
        enabled = false;
        return;
    }
    // Reset clock on first call, otherwise we do a HUGE physics sim at the start
    if ( !init ) {
        init = true;
        clock.reset();
    }

    stringstream cmd;
    cmd << "text.draw Score: " << director->getScore();
    commandList::exec( cmd.str() );

    // Step simulation according to actual time passed - should bypass framerate issues
    double dt = clock.getTimeMilliseconds();
    clock.reset();
    dynamicsWorld->stepSimulation( dt * 0.001f, 100000000, btScalar(1.)/btScalar(600.) );
    
    // Cull any objects that have fallen off the bottom of the world
    vector<Projectile*> validProjectiles;
    validProjectiles.reserve( director->getProjectiles().size() );
    while( !director->getProjectiles().empty() ) {
        Projectile* p = director->getProjectiles().back();
        if ( p->getZ() < -100 ) {
            p->removeFromWorld( dynamicsWorld );
            delete p;
        } else {
            validProjectiles.push_back( p );
        }
        director->getProjectiles().pop_back();
    }
    while( !validProjectiles.empty() ) {
        director->getProjectiles().push_back( validProjectiles.back() );
        validProjectiles.pop_back();
    }

    vector<AIUnit*> validUnits;
    validUnits.reserve( director->getUnits().size() );
    while( !director->getUnits().empty() ) {
        AIUnit* u = director->getUnits().back();
        if ( u->getZ() < -100 ) {
            u->removeFromWorld( dynamicsWorld );
            delete u;
            director->registerDeath();
        } else {
            validUnits.push_back( u );
        }
        director->getUnits().pop_back();
    }
    while( !validUnits.empty() ) {
        director->getUnits().push_back( validUnits.back() );
        validUnits.pop_back();
    }
};

void tick::callback( btDynamicsWorld *world, btScalar timeStep ) {
    director->tick();
    int numManifolds = world->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; i++ ) {
        btPersistentManifold* contactManifold = world->getDispatcher()->getManifoldByIndexInternal( i );
        GameObject* o1 = static_cast<GameObject*>( static_cast<btCollisionObject*>(
                contactManifold->getBody0() )->getUserPointer() );
        GameObject* o2 = static_cast<GameObject*>( static_cast<btCollisionObject*>(
                contactManifold->getBody1() )->getUserPointer() );

        if ( o1 == NULL || o2 == NULL )
            continue;
        
        AIUnit* ai = NULL;
        Projectile* p = NULL;
        if ( o1->getType() == AIUnit::type )
            ai = static_cast<AIUnit*>( o1 );
        else if ( o1->getType() == Projectile::type )
            p = static_cast<Projectile*>( o1 );

        if ( o2->getType() == AIUnit::type )
            ai = static_cast<AIUnit*>( o1 );
        else if ( o2->getType() == Projectile::type )
            p = static_cast<Projectile*>( o1 );

        if ( ai == NULL || p == NULL )
            continue;

        // We have a Projectile -> AI collision
        director->registerImpact();
    }
};


MK_TOOL_PLUGIN( game_init, "i", );
void game_init::click() {
    commandList::exec( "game.init" );
};

namespace game {
    MK_GUI_COMMAND(game, init, GameFactory gf; btDiscreteDynamicsWorld* initBullet();)
    void game::init( string args ) {
        GUI.ParseLine( "drawGrid=0" );
        GUI.ParseLine( "drawEdges=0" );
        GUI.ParseLine( "drawClosestEdge=0" );
        GUI.ParseLine( "drawClosestFace=0" );
        GUI.ParseLine( "textureExtractor.disable" );
        cerr << "Starting game" << endl;
        cerr << "Creating world map" << endl;
        tick::map = gf.create( environment );
        tick::dynamicsWorld = initBullet();
        gf.setupCollisionPlanes( environment, tick::dynamicsWorld );
        cerr << "Creating game director" << endl;
        tick::director = new Director( tick::dynamicsWorld, tick::map );
        cerr << "Creating renderer" << endl;
        ARDriver::mGame = new GameRenderer( tick::map, tick::director, tick::dynamicsWorld, environment );
        GUI.ParseLine( "tick.enable" );
    }

    btDiscreteDynamicsWorld* game::initBullet() {
        cerr << "Bullet init" << endl;
        int maxProxies = 1024;
        btVector3 worldAabbMin( -10000, -10000, -10000 );
        btVector3 worldAabbMax( 10000, 10000, 10000 );
        btAxisSweep3* broadphase = new btAxisSweep3( worldAabbMin, worldAabbMax, maxProxies );

        // Set up the collision configuration and dispatcher
        btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
        btCollisionDispatcher* dispatcher = new btCollisionDispatcher( collisionConfiguration );

        // The actual physics solver
        btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

        // The world.
        btDiscreteDynamicsWorld* dynamicsWorld = new btDiscreteDynamicsWorld( dispatcher, broadphase, solver, collisionConfiguration );
        dynamicsWorld->setGravity( btVector3( 0, 0, GV3::get<double>( "gravity", -10 ) ) );
        dynamicsWorld->setInternalTickCallback( tick::callback );

        btIDebugDraw* drawer = new GLDebugDrawer;
        drawer->setDebugMode( btIDebugDraw::DBG_MAX_DEBUG_DRAW_MODE );
        dynamicsWorld->setDebugDrawer( drawer );
        
        return dynamicsWorld;
    };
}

namespace ai1 {
    MK_TOOL_PLUGIN( projectile_spawn, "z", );
    void projectile_spawn::click() {
        commandList::exec( "projectile.create" );
    };
    MK_GUI_COMMAND(projectile, create,)
    void projectile::create( string params ) {
        Vector<3> camPos = environment->getCameraPose().get_translation();
        Projectile* a = tick::director->addProjectile( tick::dynamicsWorld, camPos[0], camPos[1], camPos[2] );

        Matrix<> rot = GV3::get<double>( "pushScale", 100 ) * environment->getCameraPose().get_rotation().get_matrix();
        a->push( rot[0][2], rot[1][2], rot[2][2] );
    }
}

namespace ai1 {
    MK_TOOL_PLUGIN( ai_spawn, "a", );
    void ai_spawn::click() {
        commandList::exec( "ai.create" );
    };
    MK_GUI_COMMAND(ai, create,)
    void ai::create( string params ) {
        Vector<3> aiPos = environment->getCameraPose().get_translation();
        environment->findClosestFace( aiPos );
        tick::director->addAI( tick::dynamicsWorld, aiPos[0], aiPos[1], aiPos[2]+0.1 );
    }
}

namespace ai2 {
    MK_TOOL_PLUGIN( ai_move, "w", );
    void ai_move::click() {
        cerr << "ai.move " << (tick::director->getUnits().size() - 1);
        Vector<3> target = environment->getCameraPose().get_translation();
        environment->findClosestFace( target );
        cerr << " " << target << endl;
        Waypoint* goal = new Waypoint;
        goal->x = target[0];
        goal->y = target[1];
        goal->z = target[2];
        tick::director->getUnits().back()->navigateTo( goal );
    };
    MK_GUI_COMMAND(ai, move,)
    void ai::move( string params ) {
        stringstream to( params );
        int id;
        to >> id;
        Waypoint* goal = new Waypoint;
        to >> goal->x >> goal->y >> goal->z;
        int currID = 0;
        for( vector<AIUnit*>::iterator curr = tick::director->getUnits().begin(); curr != tick::director->getUnits().end(); curr++, currID++ ) {
            if ( currID == id ) {
                (*curr)->navigateTo( goal );
            }
        }
    }
}