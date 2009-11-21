/* 
 * File:   Tool.h
 * Author: majorerror
 *
 * Created on 16 November 2009, 09:18
 */

#ifndef _TOOL_H
#define	_TOOL_H

#include "Plugin.h"
#include <cvd/image.h>
#include <cvd/rgb.h>

class Tool : public Plugin {

public:
    Tool() : Plugin() {
        Tool::list.push_back( this );
    };
    virtual ~Tool() {};
    virtual void processClick() {
        if ( enabled )
            click();
    };
    virtual std::string getName(){ return "TNULL"; };
    virtual std::string getShortName(){ return "TNULL"; };
    /**
     * A static list of all plugins currently loaded into the system, generated
     * at initialisation time.
     */
    static std::vector<Tool*> list;

protected:
    virtual void click() {};
    
};

std::vector<Tool*> Tool::list;

#define MK_TOOL_PLUGIN(TYPE,SHORTNAME) MK_SUPER_PLUGIN(TYPE,SHORTNAME,Tool, \
   virtual void click(); \
)

#endif	/* _TOOL_H */

