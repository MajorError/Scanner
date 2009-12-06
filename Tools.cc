


#include <cvd/gl_helpers.h>

#include "Environment.h"
using namespace std;
using namespace CVD;
using namespace GVars3;

MK_TOOL_PLUGIN( toolsel, "Control", void box(double l, double r, double t, double b, GLenum mode); void text(double l, double t, string s); );
void toolsel::click() {
    cout << "Select from:" << endl;
    double degsPerTool = (Tool::list.size() - 1) / 360;
    ImageRef startPt = environment->getSceneSize() * 0.5;
    startPt.y *= 0.5;
    for( unsigned int i = 0; i < Tool::list.size(); i++ ) {
        if ( Tool::list[i] == this )
            continue;
        cout << '\t' << i << ": " << Tool::list[i]->getShortName() << " [" << Tool::list[i]->getHotKey() << ']' << endl;
        // Work out layout
        double l, r, t, b;
        l = startPt.x;
        t = startPt.y;
        r = l + 50;
        b = t + 20;
        // Translucent box
        glColor4d( 0, 0.5, 0, 0.8 );
        box(l,r,t,b,GL_QUADS);
        // Opaque border
        glColor4d( 0, 1, 0, 0.8 );
        box( l,r,t,b,GL_LINE_STRIP );
        // Text
        text( l+3, t+3, Tool::list[i]->getShortName() );
        startPt.y *= 3;
    }
    unsigned int t;
    cin >> t;
    if ( t >= 0 && t < Tool::list.size() )
        GUI.ParseLine( Tool::list[t]->getShortName().append( ".activate" ) );
};

void toolsel::box(double l, double r, double t, double b, GLenum mode) {
    glBegin(mode);
    glVertex2i(l,t);
    glVertex2i(l,b);
    glVertex2i(r,b);
    glVertex2i(r,t);
    glEnd();
};

void toolsel::text(double l, double t, string s) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glTranslatef(l, t, 0.0);
    glScalef(8,-8,1);
    glDrawText(s, NICE, 1.6, 0.1);
    glPopMatrix();
}

MK_TOOL_PLUGIN( creator, "Space", );
void creator::click() {
    GUI.ParseLine( "target.create" );
};

MK_TOOL_PLUGIN( mover, "Space", );
void mover::click() {
    GUI.ParseLine( "point.move" );
};
