


#include "Tool.h"

MK_TOOL_PLUGIN( toolsel, "Control" );
void toolsel::click() {
    cout << "Select from:" << endl;
    for( unsigned int i = 0; i < Tool::list.size(); i++ ) {
        if ( Tool::list[i] == this )
            continue;
        cout << '\t' << i << ": " << Tool::list[i]->getShortName() << " [" << Tool::list[i]->getHotKey() << ']' << endl;
    }
    unsigned int t;
    cin >> t;
    if ( t >= 0 && t < Tool::list.size() )
        GUI.ParseLine( Tool::list[t]->getShortName().append( ".activate" ) );
};

MK_TOOL_PLUGIN( creator, "Space" );
void creator::click() {
    GUI.ParseLine( "target.create" );
};

MK_TOOL_PLUGIN( mover, "Space" );
void mover::click() {
    GUI.ParseLine( "point.move" );
};
