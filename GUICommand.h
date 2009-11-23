/* 
 * File:   GUICommand.h
 * Author: majorerror
 *
 * Created on 21 November 2009, 10:27
 */

#ifndef _GUICOMMAND_H
#define	_GUICOMMAND_H

#include "Plugin.h"
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
    virtual std::string getName(){ return "GNULL"; };
    virtual std::string getShortName(){ return "GNULL"; };
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

#define MK_GUI_COMMAND(NAME, COMMAND, VARS)                                 \
class NAME : public GUICommand                                              \
{                                                                           \
public:                                                                     \
    NAME() : GUICommand()                                                   \
    {                                                                       \
        setupCommands();                                                    \
        GUI.RegisterCommand( getShortName().append( "." ).append( #COMMAND ), NAME::callback, this );        \
    };                                                                      \
    virtual ~NAME() {};                                                     \
    virtual std::string getShortName()                                      \
    {                                                                       \
        return #NAME;                                                       \
    };                                                                      \
    static NAME* instance;                                                  \
protected:                                                                  \
    virtual void call( std::string params ) { COMMAND( params ); };         \
    void COMMAND( std::string params );                                     \
    VARS                                                                    \
};                                                                          \
NAME* NAME::instance = new NAME();

#endif	/* _GUICOMMAND_H */

