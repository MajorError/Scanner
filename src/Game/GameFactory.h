
#ifndef _GAMEFACTORY_H
#define	_GAMEFACTORY_H

#include "../Environment.h"
#include "WorldMap.h"

class GameFactory {
public:
    GameFactory();
    virtual ~GameFactory();
    WorldMap* create( Environment* environment );
private:

};

#endif	/* _GAMEFACTORY_H */

