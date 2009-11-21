/*
 * File:   main.cpp
 * Author: majorerror
 *
 * Created on 16 November 2009, 09:10
 */

#include <cvd/glwindow.h>
#include <stdlib.h>
#include "Environment.h"
#include "VisionProcessor.h"
#include "Tool.h"
#include "GUICommand.h"
#include "GLWindow2.h"
#include "VideoSource.h"
#include <cvd/image.h>
#include <cvd/rgb.h>
#include <cvd/byte.h>
#include "OpenGL.h"
#include <gvars3/instances.h>
#include <stdlib.h>
#include "ATANCamera.h"
#include "MapMaker.h"
#include "Tracker.h"
#include "ARDriver.h"
#include "MapViewer.h"
#include "MapPoint.h"

using namespace CVD;
using namespace std;
using namespace GVars3;

MK_VISION_PLUGIN( CameraWatcher, "cam", bool init; VideoSource videoSource; );
void CameraWatcher::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    if ( !init ) {
        init = true;
        sceneBW.resize( videoSource.Size() );
        sceneRGB.resize( videoSource.Size() );
    }
    videoSource.GetAndFillFrameBWandRGB( sceneBW, sceneRGB );
};

MK_VISION_PLUGIN( PTAMDispatcher, "ptam", bool init; Map *mpMap; \
  MapMaker *mpMapMaker; Tracker *mpTracker; ATANCamera *mpCamera; );
void PTAMDispatcher::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    if ( !init ) {
        GUI.LoadFile( "settings.cfg" );
        init = true;
        Vector<NUMTRACKERCAMPARAMETERS> vTest;

        vTest = GV3::get< Vector<NUMTRACKERCAMPARAMETERS> >( "Camera.Parameters", ATANCamera::mvDefaultParams, HIDDEN );
        mpCamera = new ATANCamera( "Camera" );
        if( vTest == ATANCamera::mvDefaultParams ) {
          cout << endl;
          cout << "! Camera.Parameters is not set, need to run the CameraCalibrator tool" << endl;
          cout << "  and/or put the Camera.Parameters= line into the appropriate .cfg file." << endl;
          exit(1);
        }

        mpMap = new Map;
        mpMapMaker = new MapMaker( *mpMap, *mpCamera );
        mpTracker = new Tracker( sceneBW.size(), *mpCamera, *mpMap, *mpMapMaker );

        environment->setCamera( mpCamera );
    }

    mpTracker->TrackFrame( sceneBW, false/*!mpMap->IsGood()*/ );
    environment->setCameraPose( mpTracker->GetCurrentPose() );
    // TODO: Is this slow? Worth it to prevent dep leakage?
    environment->clearFeatures();
    for( unsigned int i = 0; i < mpMap->vpPoints.size(); i++ )
        if ( !mpMap->vpPoints[i]->bBad )
            environment->addFeature( mpMap->vpPoints[i]->v3WorldPos );
};

MK_VISION_PLUGIN( GUIDispatcher, "guid", bool init; GLWindow2 *glWindow; ARDriver *ard; );
void GUIDispatcher::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    if ( !init ) {
        init = true;
        glWindow = new GLWindow2( sceneBW.size(), "Handheld 3D Scanner" );
        GLWindow2 &glw = *glWindow;
        ATANCamera &camera = *environment->getCamera();
        ard = new ARDriver( camera, sceneRGB.size(), glw, environment );

        GUI.ParseLine("GLWindow.AddMenu Menu Menu");
        GUI.ParseLine("Menu.ShowMenu Root");
    }

    glWindow->SetupViewport();
    glWindow->SetupVideoOrtho();
    glWindow->SetupVideoRasterPosAndZoom();

    ard->Render( sceneRGB, environment->getCameraPose() );

    // (TODO?) No message at this time... glWindow.DrawCaption( sCaption );
    glWindow->DrawMenus();
    glWindow->swap_buffers();
    glWindow->HandlePendingEvents();
};

/*MK_TOOL_PLUGIN( Clicky, "clk" );
void Clicky::click() {
    cout << "CLICK" << endl;
};*/

MK_GUI_COMMAND( point, create )
void point::create( string params ) {
    Vector<3> v;
    if ( params.size() > 0 ) {
        double d[3];
        std::stringstream paramStream( params );
        paramStream >> d[0];
        paramStream >> d[1];
        paramStream >> d[2];
        v = makeVector( d[0], d[1], d[2] );
    } else {
        v = environment->getCameraPose().get_translation();
    }
    environment->addPoint( v );
}

/*
 *
 */
int main(int argc, char** argv) {
    CVD::Image<CVD::byte> bw;
    CVD::Image< CVD::Rgb<CVD::byte> > rgb;

    GVars3::GUI.StartParserThread();

    Environment env;
    VisionPlugin::setEnvironment( &env );
    GUICommand::setEnvironment( &env );

    while( true ) {
        for( unsigned int i = 0; i < VisionPlugin::list.size(); i++ ) {
            VisionPlugin::list[i]->process( bw, rgb );
        }
    }
    return (EXIT_SUCCESS);
}

