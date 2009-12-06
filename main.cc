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

int main(int argc, char** argv) {
    Image<CVD::byte> bw;
    Image< CVD::Rgb<CVD::byte> > rgb;

    //GUI.StartParserThread();

    Environment env;
    VisionPlugin::setEnvironment( &env );
    GUICommand::setEnvironment( &env );
    Tool::setEnvironment( &env );

    // Main system loop
    while( true ) {
        for( unsigned int i = 0; i < VisionPlugin::list.size(); i++ ) {
            VisionPlugin::list[i]->process( bw, rgb );
        }
        //if ( !cin.eof() )
        //    GUI.ParseStream( cin );
    }
    return (EXIT_SUCCESS);
}

