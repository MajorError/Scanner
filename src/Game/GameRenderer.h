
#ifndef _GAMERENDERER_H
#define	_GAMERENDERER_H

#include <TooN/TooN.h>
#include "../ARPointRenderer.h"
#include "WorldMap.h"
#include "Director.h"

using namespace TooN;

class GameRenderer : public ARPointRenderer {
public:
    GameRenderer( WorldMap* m, Director* d,Environment* e );
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
};

#endif	/* _GAMERENDERER_H */

