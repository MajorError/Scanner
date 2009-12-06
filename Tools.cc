


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
        curr++;
        curr = std::min( (int)Tool::list.size()-1, std::max( 2, curr % (int)Tool::list.size() ) );
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
    commandList::exec( "target.create" );
};

MK_TOOL_PLUGIN( mover, "Space", );
void mover::click() {
    commandList::exec( "point.move" );
};

MK_TOOL_PLUGIN( connect, "Space", );
void connect::click() {
    commandList::exec( "edge.connect" );
};
