


#include <cvd/gl_helpers.h>

#include "Environment.h"
#include "Tool.h"
using namespace std;
using namespace CVD;
using namespace GVars3;

MK_TOOL_PLUGIN( toolsel, "Control", bool init; pthread_t display; static void* displayTimeout(void* ptr);\
public: int curr; bool running; time_t start; );
void toolsel::click() {
    start = time( NULL );
    if ( !init ) {
        init = true;
        curr = 0;
    }
    if( !running ) {
        curr = 2;
        running = true;
        pthread_create( &display, NULL, toolsel::displayTimeout, (void*)this );
    } else {
        curr = std::min( (int)Tool::list.size()-1, std::max( 2, (curr+1) % (int)Tool::list.size() ) );
        while( Tool::list[curr]->getHotKey() != "Space" )
            curr = std::min( (int)Tool::list.size()-1, std::max( 2, (curr+1) % (int)Tool::list.size() ) );
    }
    commandList::exec( "toolselproc.activate" );
};

void* toolsel::displayTimeout( void* ptr ) {
    toolsel* ts = static_cast<toolsel*>( ptr );
    while( ts->running && time( NULL ) < ts->start + 10 ) {
        commandList::exec( "text.draw " + Tool::list[ts->curr]->getShortName() );
        usleep(10);
    }
    ts->running = false;
    
    commandList::exec( "toolselproc.disable" );
    return ptr;
};

MK_TOOL_PLUGIN( toolselproc, "Space", );
void toolselproc::click() {
    toolsel* ts = static_cast<toolsel*>( Tool::list[0] );
    ts->running = false;
    commandList::exec( Tool::list[ts->curr]->getShortName().append( ".activate" ) );
};

MK_TOOL_PLUGIN( creator, "Space", );
void creator::click() {
    commandList::exec( "vertex.create" );
};

MK_TOOL_PLUGIN( mover, "Space", );
void mover::click() {
    commandList::exec( "vertex.move" );
};

MK_TOOL_PLUGIN( connect, "Space", );
void connect::click() {
    commandList::exec( "edge.connect" );
};

MK_TOOL_PLUGIN( bisect, "Space", );
void bisect::click() {
    commandList::exec( "edge.bisect" );
};

MK_TOOL_PLUGIN( toggleBG, "b", );
void toggleBG::click() {
    commandList::exec( string( "drawBackground=" )+(GV3::get<bool>( "drawBackground", true ) ? "0" : "1") );
};

MK_TOOL_PLUGIN( toggleModel, "m", );
void toggleModel::click() {
    commandList::exec( string( "drawModel=" )+(GV3::get<bool>( "drawModel", true ) ? "0" : "1") );
};

MK_TOOL_PLUGIN( toggleVertices, "v", );
void toggleVertices::click() {
    commandList::exec( string( "drawPoints=" )+(GV3::get<bool>( "drawPoints", true ) ? "0" : "1") );
};

MK_TOOL_PLUGIN( toggleGrid, "g", );
void toggleGrid::click() {
    commandList::exec( string( "drawGrid=" )+(GV3::get<bool>( "drawGrid", true ) ? "0" : "1") );
};