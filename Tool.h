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
        enabled = false;
    };
    virtual ~Tool() {};
    static void callback( void* obj, std::string cmd, std::string params ) {
        if ( cmd.substr( cmd.length() - 8 ) == "activate" ) {
            for( unsigned int i = 0; i < Tool::list.size(); i++ )
                Tool::list[i]->enabled = false;
            static_cast<Tool*>( obj )->enabled = true;
        } else {
            static_cast<Tool*>( obj )->processClick();
        }
    };
    /**
     * We will always process the click if we are explicitly enabled.
     * Beyond this, if we are the only Tool assigned a given hotkey, we will
     * process regardless of the state of enabled; activation only modifies
     * behaviour for overloaded keys
     */
    void processClick() {
        if ( !enabled )
            for( unsigned int i = 0; i < Tool::list.size(); i++ )
                if ( Tool::list[i] != this && Tool::list[i]->getHotKey() == getHotKey() )
                    return;
        click();
    };
    virtual std::string getShortName() { return "TNULL"; };
    virtual std::string getHotKey() { return "TNULL"; };
    bool isEnabled() {
        return enabled;
    };
    /**
     * A static list of all plugins currently loaded into the system, generated
     * at initialisation time.
     */
    static std::vector<Tool*> list;
    static void setEnvironment( Environment *e ) {
        for( unsigned int i = 0; i < list.size(); i++ )
            list[i]->environment = e;
    };

protected:
    Environment *environment;
    virtual void click() {};
    
};

std::vector<Tool*> Tool::list;

#define MK_TOOL_PLUGIN(TYPE,HOTKEY,VARS) MK_SUPER_PLUGIN(TYPE,TYPE,Tool,    \
    virtual void click();                                                   \
    virtual std::string getHotKey(){ return HOTKEY; };                      \
    virtual void setupCommands()                                            \
    {                                                                       \
        Plugin::setupCommands();                                            \
        GUI.RegisterCommand( getShortName().append( ".activate" ), Tool::callback, this ); \
    };                                                                      \
protected:                                                                  \
    VARS                                                                    \
)

#endif	/* _TOOL_H */

