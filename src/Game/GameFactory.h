
#ifndef _GAMEFACTORY_H
#define	_GAMEFACTORY_H

#include "../Environment.h"
#include "WorldMap.h"
#include <btBulletDynamicsCommon.h>

class GameFactory {
public:
    GameFactory();
    virtual ~GameFactory();
    WorldMap* create( Environment* environment );
    void setupCollisionPlanes( Environment* env, btDiscreteDynamicsWorld* world );
private:

};

#endif	/* _GAMEFACTORY_H */

