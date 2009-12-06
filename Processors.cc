


#include <cvd/image.h>

MK_VISION_PLUGIN( cam, bool init; VideoSource videoSource; );
void cam::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    if ( !init ) {
        init = true;
        sceneBW.resize( videoSource.Size() );
        sceneRGB.resize( videoSource.Size() );
    }
    videoSource.GetAndFillFrameBWandRGB( sceneBW, sceneRGB );
};

MK_VISION_PLUGIN( ptam, bool init; Map *mpMap; \
  MapMaker *mpMapMaker; Tracker *mpTracker; ATANCamera *mpCamera; );
void ptam::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
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
    // TODO: Is this slow? Worth it to prevent dep leakage?
    environment->clearFeatures();
    for( unsigned int i = 0; i < mpMap->vpPoints.size(); i++ )
        if ( !mpMap->vpPoints[i]->bBad )
            environment->addFeature( mpMap->vpPoints[i]->v3WorldPos );
};

MK_VISION_PLUGIN( guiDispatch, bool init; GLWindow2 *glWindow; ARDriver *ard; );
void guiDispatch::doProcessing( Image<byte>& sceneBW, Image< Rgb<byte> >& sceneRGB ) {
    if ( !init ) {
        init = true;
        glWindow = new GLWindow2( sceneBW.size(), "Handheld 3D Scanner" );
        GLWindow2 &glw = *glWindow;
        ATANCamera &camera = *environment->getCamera();
        ard = new ARDriver( camera, sceneRGB.size(), glw, environment );
        environment->setSceneSize( sceneRGB.size() );

        GUI.ParseLine("GLWindow.AddMenu Menu Menu");
        GUI.ParseLine("Menu.ShowMenu Root");
    }

    glWindow->SetupViewport();
    glWindow->SetupVideoOrtho();
    glWindow->SetupVideoRasterPosAndZoom();

    ard->Render( sceneRGB, environment->getCameraPose() );

    // (TODO?) No message at this time... glWindow->DrawCaption( sCaption );
    glWindow->DrawMenus();
    glWindow->swap_buffers();
    glWindow->HandlePendingEvents();
};
