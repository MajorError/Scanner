/* 
 * File:   Plugin.h
 * Author: majorerror
 *
 * Created on 16 November 2009, 09:13
 */

#ifndef _PLUGIN_H
#define	_PLUGIN_H

#include <vector>
#include <string>
#include <gvars3/instances.h>

/**
 * This is essentially just an abstract interface for all the plugins to
 * conform to, defining the available types of operation.
 *
 * Each implementation of this class should only be a singleton.
 */
class Plugin
{
public:
    Plugin() {
        enabled = true;
    };
    virtual ~Plugin() {};
    static void enable( void* ptr, std::string cmd, std::string params ) {
        static_cast<Plugin*>( ptr )->enabled = true;
    };
    static void disable( void* ptr, std::string cmd, std::string params ) {
        static_cast<Plugin*>( ptr )->enabled = false;
    };

    virtual void setupCommands() {
        GVars3::GUI.RegisterCommand( getShortName().append( ".enable" ), Plugin::enable, this );
        GVars3::GUI.RegisterCommand( getShortName().append( ".disable" ), Plugin::disable, this );
    };
    /**
     * Answer a name for this plugin; should make it clear what this plugin
     * does, how it affects the output, and what its parameters are.
     * \return A UI-friendly string name
     */
    virtual std::string getName(){ return "NULL"; };
    /**
     * Answer a short-name for this plugin; should be short enough to be used
     * in scripting to choose the operation
     * \return A code-friendly short string name
     */
    virtual std::string getShortName(){ return "NULL"; };

protected:
    bool enabled;
};

/**
 * A C++ Macro to define and load a  plugin class trivially.
 */
#define MK_SUPER_PLUGIN(TYPE,SHORTNAME,SUPER,DECLS)                         \
class TYPE : public SUPER                                                   \
{                                                                           \
public:                                                                     \
    TYPE() : SUPER()                                                        \
    {                                                                       \
        setupCommands();                                                    \
    };                                                                      \
    virtual ~TYPE() {};                                                     \
    virtual std::string getName()                                           \
    {                                                                       \
        return "TYPE";                                                      \
    };                                                                      \
    virtual std::string getShortName()                                      \
    {                                                                       \
        return SHORTNAME;                                                   \
    };                                                                      \
    static TYPE* instance;                                                  \
    DECLS                                                                   \
};                                                                          \
TYPE* TYPE::instance = new TYPE();
// end MK_SUPER_PLUGIN


#endif	/* _PLUGIN_H */

