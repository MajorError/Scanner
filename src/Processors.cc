#include <cvd/image.h>
#include <gvars3/instances.h>

#include "VisionProcessor.h"
#include "Environment.h"
#include "Shiny.h"
#include "Map.h"
#include <sys/time.h>
#include <cvd/gl_helpers.h>
#include <cvd/convolution.h>

using namespace CVD;
using namespace GVars3;
using namespace TooN;
using namespace std;

MK_VISION_PLUGIN( cam, VideoSource videoSource; );
void cam::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    PROFILE_BEGIN( cam );
    if ( !init ) {
        init = true;
        sceneBW.resize( videoSource.Size() );
        sceneRGB.resize( videoSource.Size() );
    }
    videoSource.GetAndFillFrameBWandRGB( sceneBW, sceneRGB );
    PROFILE_END();
};

MK_VISION_PLUGIN( ptam, Map *mpMap; \
  MapMaker *mpMapMaker; Tracker *mpTracker; ATANCamera *mpCamera; );
void ptam::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    PROFILE_BEGIN( ptam );
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
    environment->setCameraPose( mpTracker->GetCurrentPose().inverse() );
    if( !mpMap->IsGood() )
        GUI.ParseLine( "text.draw Perform stereo init to start modelling" );
    environment->clearFeatures();
    for( unsigned int i = 0; i < mpMap->vpPoints.size(); i++ )
        if ( !mpMap->vpPoints[i]->bBad )
            environment->addFeature( mpMap->vpPoints[i]->v3WorldPos );
    PROFILE_END();
};

MK_VISION_PLUGIN( guiDispatch, GLWindow2 *glWindow; ARDriver *ard; );
void guiDispatch::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    PROFILE_BEGIN( gui );
    if ( !init ) {
        init = true;
        glWindow = new GLWindow2( sceneBW.size(), "Handheld 3D Scanner" );
        GLWindow2 &glw = *glWindow;
        ATANCamera &camera = *environment->getCamera();
        ard = new ARDriver( camera, sceneRGB.size(), glw, environment );
        environment->setSceneSize( sceneRGB.size() );
    }

    glWindow->swap_buffers();
    glWindow->SetupViewport();
    glWindow->SetupVideoOrtho();
    glWindow->SetupVideoRasterPosAndZoom();

    ard->Render( sceneRGB, environment->getCameraPose() );

    // (TODO?) No message at this time... glWindow->DrawCaption( sCaption );
    glWindow->DrawMenus();
    glWindow->HandlePendingEvents();
    PROFILE_END();
};

MK_VISION_PLUGIN( commandList, static vector<string> commands; static pthread_mutex_t mutex; public: static void exec( string cmd ); );
void commandList::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    PROFILE_BEGIN( commands );
    if ( !init ) {
        init = true;
    }

    pthread_mutex_lock( &mutex );
    for( unsigned int i = 0; i < commands.size(); i++ ) {
        GUI.ParseLine( commands[i] );
    }
    commands.clear();
    pthread_mutex_unlock( &mutex );
    PROFILE_END();
};

void commandList::exec( string cmd ) {
    pthread_mutex_lock( &mutex );
    commands.push_back( cmd );
    pthread_mutex_unlock( &mutex );
};
vector<string> commandList::commands;
pthread_mutex_t commandList::mutex = PTHREAD_MUTEX_INITIALIZER;

MK_VISION_PLUGIN( textureExtractor, );
void textureExtractor::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    PROFILE_BEGIN( textures );
    SE3<> camera = environment->getCameraPose();
    for( set<PolyFace*>::iterator it = environment->getFaces().begin();
            it != environment->getFaces().end(); it++ ) {
        (*it)->testAndSetTexture( sceneRGB, camera, environment->getCamera() );
    }
    PROFILE_END();
};

MK_VISION_PLUGIN( profiler, );
void profiler::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    PROFILE_UPDATE_ALL( 0 );
    PROFILE_OUTPUT_ALL();
    PROFILE_DESTROY_ALL();
};

MK_VISION_PLUGIN( fps, timeval lastTime; int ctr; );
void fps::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    timeval currTime;
    gettimeofday( &currTime, NULL );
    ctr = (ctr % 30) + 1;
    // Average time in ms between frames
    double ms = ((currTime.tv_sec - lastTime.tv_sec) * 1000 +
			(currTime.tv_usec - lastTime.tv_usec) / 1000) / ctr;
    stringstream s;
    s << "text.draw " << floor( 1000 / ms ) << " FPS";
    commandList::exec( s.str() );
    if ( ctr == 1 ) {
        lastTime.tv_sec = currTime.tv_sec;
        lastTime.tv_usec = currTime.tv_usec;
    }
};

MK_VISION_PLUGIN( accuracy, int ctr; public: Image< Rgb<byte> > rendered; );
void accuracy::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    if ( !init ) {
        init = true;
        rendered.resize( sceneRGB.size() );
        ctr = 0;
    }

    if ( ctr++ % 30 )
        return;
    
    glReadPixels( rendered );

    if ( GV3::get<bool>( "blurAccuracy", true ) ) {
        convolveGaussian( rendered, 1.0 );
        convolveGaussian( sceneRGB, 1.0 );
    }

    double diff = 0;
    Image< Rgb<byte> >::iterator camPx = sceneRGB.begin();
    Image< Rgb<byte> >::iterator arPx = rendered.end() - 1;
    for( ; arPx >= rendered.begin(); arPx -= rendered.size().x ) {
        for( int i = rendered.size().x - 1; i >= 0; i-- ) {
            double currDiff = (abs( camPx->red - (arPx-i)->red ) 
                    + abs( camPx->green - (arPx-i)->green )
                    + abs( camPx->blue - (arPx-i)->blue )) / 3.0;
            camPx++;

            diff += currDiff;
            (arPx-i)->red = currDiff;
            (arPx-i)->green = currDiff;
            (arPx-i)->blue = currDiff;
        }
    }
    
    cerr << "Average error: " << (100.0 * diff / (256 * rendered.size().x * rendered.size().y)) << "%" << endl;
};
