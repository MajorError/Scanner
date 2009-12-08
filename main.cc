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
#include <algorithm>
#include "ATANCamera.h"
#include "MapMaker.h"
#include "Tracker.h"
#include "ARDriver.h"
#include "MapViewer.h"
#include "MapPoint.h"

using namespace CVD;
using namespace std;
using namespace GVars3;

#include "Processors.cc"
#include "Commands.cc"
#include "Tools.cc"


 void* safeCommandParser(void* ptr) {
     string line;
     while( true ) {
         cout << "> ";
         cin >> line;
         commandList::exec( line );
     }
     return ptr;
 };

void callback( void* obj, std::string cmd, std::string params ) {
    if( cmd == "exit" || cmd == "quit" ) {
        exit(0);
    }
};


int main(int argc, char** argv) {
    Image<CVD::byte> bw;
    Image< CVD::Rgb<CVD::byte> > rgb;

    Environment env;
    VisionPlugin::setEnvironment( &env );
    GUICommand::setEnvironment( &env );
    Tool::setEnvironment( &env );

    pthread_t commandParser;
    pthread_create( &commandParser, NULL, safeCommandParser, NULL );

    GUI.RegisterCommand( "exit", &callback );
    GUI.RegisterCommand( "quit", &callback );

    // Main system loop
    while( true ) {
        for( unsigned int i = 0; i < VisionPlugin::list.size(); i++ ) {
            VisionPlugin::list[i]->process( bw, rgb );
        }
    }
    return EXIT_SUCCESS;
}

