


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
        for ( curr = Tool::list.size()-1; curr > 2; curr-- )
            if ( Tool::list[curr]->isEnabled() )
                break;
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

MK_TOOL_PLUGIN( point_mover, "Space", );
void point_mover::click() {
    commandList::exec( "vertex.move" );
};

MK_TOOL_PLUGIN( connect, "Space", );
void connect::click() {
    commandList::exec( "edge.connect" );
};

MK_TOOL_PLUGIN( plane_mover, "Space", );
void plane_mover::click() {
    commandList::exec( "plane.move" );
};

MK_TOOL_PLUGIN( edge_bisect, "Space", bool startBisect;);
void edge_bisect::click() {
    // Thanks to randomly assigned values in memory locations (startBisect is
    //  uninitialised), we have to assign values directly here.
    if ( startBisect ) {
        commandList::exec( "edge.bisect" );
        commandList::exec( "vertex.move" );
        startBisect = false;
    } else {
        commandList::exec( "vertex.move" );
        startBisect = true;
    }
};

MK_TOOL_PLUGIN( edge_remove, "Space", );
void edge_remove::click() {
    commandList::exec( "edge.remove" );
};

MK_TOOL_PLUGIN( extrude, "Space", bool startExtrude;);
void extrude::click() {
    if ( startExtrude ) {
        commandList::exec( "plane.extrude" );
        commandList::exec( "plane.move" );
        startExtrude = false;
    } else {
        commandList::exec( "plane.move" );
        startExtrude = true;
    }
};

MK_TOOL_PLUGIN( vertex_remove, "Space", );
void vertex_remove::click() {
    commandList::exec( "vertex.remove" );
};

MK_TOOL_PLUGIN( texture_clean, "Space", );
void texture_clean::click() {
    commandList::exec( "texture.clean" );
};

MK_TOOL_PLUGIN( texture_delete, "Space", );
void texture_delete::click() {
    commandList::exec( "texture.clear" );
};

MK_TOOL_PLUGIN( shrink_wrap, "Space", );
void shrink_wrap::click() {
    commandList::exec( "shrinkwrap.exec" );
};

namespace toggles {

    void* displayTimeout( void* ptr ) {
        string* text = static_cast<string*>( ptr );
        time_t start = time( NULL );
        while( time( NULL ) < start + 1 ) {
            commandList::exec( "text.draw " + *text );
            usleep(10);
        }
        delete text;
        return NULL;
    };

    void toggleVar( string varName ) {
        string* text = new string( "Toggle " + varName + " to " + (GV3::get<bool>( varName, true ) ? "0" : "1") );
        pthread_t t;
        commandList::exec( varName+"="+(GV3::get<bool>( varName, true ) ? "0" : "1") );
        pthread_create( &t, NULL, displayTimeout, text );
    };
    MK_TOOL_PLUGIN( toggleBG, "b", );
    void toggleBG::click() {
        toggleVar( "drawBackground" );
    };

    MK_TOOL_PLUGIN( toggleModel, "m", );
    void toggleModel::click() {
        toggleVar( "drawModel" );
    };

    MK_TOOL_PLUGIN( toggleVertices, "v", );
    void toggleVertices::click() {
        toggleVar( "drawPoints" );
    };

    MK_TOOL_PLUGIN( toggleEdges, "e", );
    void toggleEdges::click() {
        toggleVar( "drawEdges" );
    };

    MK_TOOL_PLUGIN( toggleGrid, "g", );
    void toggleGrid::click() {
        toggleVar( "drawGrid" );
    };
}