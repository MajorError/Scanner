/* 
 * File:   VisionProcessor.h
 * Author: majorerror
 *
 * Created on 16 November 2009, 09:18
 */

#ifndef _VISIONPROCESSOR_H
#define	_VISIONPROCESSOR_H

#include "Plugin.h"
#include <cvd/image.h>
#include <cvd/rgb.h>

class VisionPlugin : public Plugin {

public:
    VisionPlugin() : Plugin() {
        VisionPlugin::list.push_back( this );
    };
    virtual ~VisionPlugin() {};
    virtual void process( CVD::Image<CVD::byte>& sceneBW, CVD::Image< CVD::Rgb<CVD::byte> >& sceneRGB ) {
        if ( enabled )
            doProcessing( sceneBW, sceneRGB );
    };
    virtual std::string getName(){ return "VNULL"; };
    virtual std::string getShortName(){ return "VNULL"; };
    /**
     * A static list of all plugins currently loaded into the system, generated
     * at initialisation time.
     */
    static std::vector<VisionPlugin*> list;
    static void setEnvironment( Environment *e ) {
        for( unsigned int i = 0; i < list.size(); i++ )
            list[i]->environment = e;
    };

protected:
    Environment *environment;
    virtual void doProcessing( CVD::Image<CVD::byte>& sceneBW, CVD::Image< CVD::Rgb<CVD::byte> >& sceneRGB ) {};
    
};

std::vector<VisionPlugin*> VisionPlugin::list;

#define MK_VISION_PLUGIN(TYPE,SHORTNAME,VARS) MK_SUPER_PLUGIN(TYPE,SHORTNAME,VisionPlugin, \
   virtual void doProcessing( CVD::Image<CVD::byte>& sceneBW, CVD::Image< CVD::Rgb<CVD::byte> >& sceneRGB ); \
protected:              \
    VARS                \
)

#endif	/* _VISIONPROCESSOR_H */

