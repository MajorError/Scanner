
#ifndef _GAMERENDERER_H
#define	_GAMERENDERER_H

#include <TooN/TooN.h>
#include "../ARPointRenderer.h"
#include "WorldMap.h"

using namespace TooN;

class GameRenderer : public ARPointRenderer {
public:
    GameRenderer( WorldMap* m, Environment* e );
    virtual ~GameRenderer();
    virtual void DrawStuff( SE3<> camera );

    void renderWaypointGraph();
    void renderUnits();
private:
    WorldMap* map;

};

#endif	/* _GAMERENDERER_H */

