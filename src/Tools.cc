


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
        /* Actually seems more intuitive to start at the beginning of the list
         for ( curr = Tool::list.size()-1; curr > 2; curr-- )
            if ( Tool::list[curr]->isEnabled() )
                break;*/
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

MK_TOOL_PLUGIN( point_freemove, "Space", );
void point_freemove::click() {
    commandList::exec( "vertex.move" );
};

MK_TOOL_PLUGIN( point_X_move, "Space", );
void point_X_move::click() {
    commandList::exec( "vertex.move 1 0 0" );
};

MK_TOOL_PLUGIN( point_Y_move, "Space", );
void point_Y_move::click() {
    commandList::exec( "vertex.move 0 1 0" );
};

MK_TOOL_PLUGIN( point_Z_move, "Space", );
void point_Z_move::click() {
    commandList::exec( "vertex.move 0 0 1" );
};

MK_TOOL_PLUGIN( connect, "Space", );
void connect::click() {
    commandList::exec( "edge.connect" );
};

MK_TOOL_PLUGIN( plane_mover, "Space", );
void plane_mover::click() {
    commandList::exec( "plane.move" );
};

MK_TOOL_PLUGIN( plane_split, "Space",);
void plane_split::click() {
    if ( !init )
        commandList::exec( "plane.split" );
    commandList::exec( "vertex.move" );
    init = !init;
};

MK_TOOL_PLUGIN( edge_bisect, "Space",);
void edge_bisect::click() {
    if ( !init )
        commandList::exec( "edge.bisect" );
    commandList::exec( "vertex.move" );
    init = !init;
};

MK_TOOL_PLUGIN( edge_remove, "Space", );
void edge_remove::click() {
    commandList::exec( "edge.remove" );
};

MK_TOOL_PLUGIN( extrude, "Space",);
void extrude::click() {
    if ( !init )
        commandList::exec( "plane.extrude" );
    commandList::exec( "plane.move" );
    init = !init;
};

MK_TOOL_PLUGIN( revolve, "Space", );
void revolve::click() {
    commandList::exec( "plane.revolve" );
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

#ifdef USE_EXPERIMENTALS
MK_TOOL_PLUGIN( shrink_wrap, "Space", );
void shrink_wrap::click() {
    commandList::exec( "shrinkwrap.exec" );
};
#endif

MK_TOOL_PLUGIN( subdivide, "Space", );
void subdivide::click() {
    commandList::exec( "mesh.subdivide" );
};

MK_TOOL_PLUGIN( smooth_deform, "Space", );
void smooth_deform::click() {
    commandList::exec( "mesh.smoothDeform" );
};

MK_TOOL_PLUGIN( scale, "Space", );
void scale::click() {
    commandList::exec( "mesh.scale" );
};

MK_TOOL_PLUGIN( mesh_move, "Space", );
void mesh_move::click() {
    commandList::exec( "mesh.move" );
};

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

MK_TOOL_PLUGIN( save, "s", );
void save::click() {
    pthread_t t;
    string* text = new string( "Saving Wavefront OBJ File" );
    commandList::exec( "text.draw " + *text );
    pthread_create( &t, NULL, displayTimeout, text );
    commandList::exec( "obj.save" );
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

MK_TOOL_PLUGIN( toggleFaces, "f", );
void toggleFaces::click() {
    toggleVar( "drawFaces" );
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

MK_TOOL_PLUGIN( toggleNormals, "n", );
void toggleNormals::click() {
    toggleVar( "drawNormals" );
};

MK_TOOL_PLUGIN( toggleProfiler, "p", );
void toggleProfiler::click() {
    if ( profiler::instance->enabled )
        profiler::disable( profiler::instance );
    else
        profiler::enable( profiler::instance );
};

MK_TOOL_PLUGIN( clearGUI, "0", );
void clearGUI::click() {
    GUI.ParseLine( "fps.disable" );
    GUI.ParseLine( "drawGrid=0" );
    GUI.ParseLine( "drawEdges=0" );
    GUI.ParseLine( "drawPoints=0" );
    GUI.ParseLine( "drawFeatures=0" );
    GUI.ParseLine( "drawTarget=0" );
    GUI.ParseLine( "drawClosestPoint=0" );
    GUI.ParseLine( "drawClosestEdge=0" );
    GUI.ParseLine( "drawClosestFace=0" );
};

MK_TOOL_PLUGIN( restoreGUI, "9", );
void restoreGUI::click() {
    GUI.ParseLine( "fps.enable" );
    GUI.ParseLine( "drawGrid=1" );
    GUI.ParseLine( "drawEdges=1" );
    GUI.ParseLine( "drawPoints=1" );
    GUI.ParseLine( "drawFeatures=1" );
    GUI.ParseLine( "drawTarget=1" );
    GUI.ParseLine( "drawClosestPoint=1" );
    GUI.ParseLine( "drawClosestEdge=1" );
    GUI.ParseLine( "drawClosestFace=1" );
};

MK_TOOL_PLUGIN( saveErrorMap, "1", );
void saveErrorMap::click() {
    commandList::exec( "errormap.save" );
};