/* 
 * File:   GUICommand.h
 * Author: majorerror
 *
 * Created on 21 November 2009, 10:27
 */

#ifndef _GUICOMMAND_H
#define	_GUICOMMAND_H

#include "Plugin.h"
#include <gvars3/instances.h>

using namespace GVars3;

class GUICommand : public Plugin {

public:
    GUICommand() : Plugin() {
        GUICommand::list.push_back( this );
    };
    virtual ~GUICommand() {};
    static void callback( void* obj, std::string cmd, std::string params ) {
        if ( static_cast<GUICommand*>( obj )->enabled )
            static_cast<GUICommand*>( obj )->call( params );
    };
    virtual std::string getShortName() { return "CNULL"; };
    /**
     * A static list of all plugins currently loaded into the system, generated
     * at initialisation time.
     */
    static std::vector<GUICommand*> list;
    static void setEnvironment( Environment *e ) {
        for( unsigned int i = 0; i < list.size(); i++ )
            list[i]->environment = e;
    };

protected:
    Environment *environment;
    virtual void call( std::string params ) {};

};

std::vector<GUICommand*> GUICommand::list;

#define MK_GUI_COMMAND(NAME, COMMAND, VARS) MK_SUPER_PLUGIN(NAME,NAME,GUICommand, \
    virtual void setupCommands()                                            \
    {                                                                       \
        Plugin::setupCommands();                                            \
        GUI.RegisterCommand( getShortName().append( "." ).append( #COMMAND ), GUICommand::callback, this ); \
    };                                                                      \
protected:                                                                  \
    virtual void call( std::string params ) { COMMAND( params ); };         \
    void COMMAND( std::string params );                                     \
    VARS                                                                    \
)

#endif	/* _GUICOMMAND_H */

