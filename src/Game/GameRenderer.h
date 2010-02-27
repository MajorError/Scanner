
#ifndef _GAMERENDERER_H
#define	_GAMERENDERER_H

#include <TooN/TooN.h>
#include "../ARPointRenderer.h"
#include "WorldMap.h"
#include "Director.h"
#include "BulletDynamics/btBulletDynamicsCommon.h"

using namespace TooN;

class GameRenderer : public ARPointRenderer {
public:
    GameRenderer( WorldMap* m, Director* d, btDynamicsWorld* dw, Environment* e );
    virtual ~GameRenderer();
    virtual void DrawStuff( SE3<> camera );

    void renderWaypointGraph();
    void renderProjectiles();
    void renderUnits();
protected:
    void DrawCube();
private:
    WorldMap* map;
    Director* director;
    btDynamicsWorld* dynamicsWorld;
};

#endif	/* _GAMERENDERER_H */

