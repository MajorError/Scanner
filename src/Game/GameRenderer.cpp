
#include "GameRenderer.h"
#include "AIUnit.h"

GameRenderer::GameRenderer( WorldMap* m, Environment* e ) : ARPointRenderer( e ), map( m ) {
};

GameRenderer::~GameRenderer() {
};

void GameRenderer::DrawStuff( SE3<> camera ) {
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );
    glEnable( GL_NORMALIZE );
    glEnable( GL_COLOR_MATERIAL );

    GLfloat af[4];
    af[0] = 0.5;
    af[1] = 0.5;
    af[2] = 0.5;
    af[3] = 1.0;
    glLightfv(GL_LIGHT0, GL_AMBIENT, af);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, af);
    af[0] = 1.0;
    af[1] = 0.0;
    af[2] = 1.0;
    af[3] = 0.0;
    glLightfv(GL_LIGHT0, GL_POSITION, af);
    af[0] = 1.0;
    af[1] = 1.0;
    af[2] = 1.0;
    af[3] = 1.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, af);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0);

    glMatrixMode(GL_MODELVIEW);

    cerr << "Game Render" << endl;

    DrawPolys();
    if ( GV3::get<bool>( "drawWaypoints", true ) )
        renderWaypointGraph();
    renderUnits();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
};

void GameRenderer::renderWaypointGraph() {

};

void GameRenderer::renderUnits() {
    double ds = GV3::get<double>( "ptSize", 0.05 );
    /*for( list<AIUnit*>::iterator curr = map->getUnits().begin(); curr != map->getUnits().end(); curr++ ) {
        glColor4d(1.0, 0.2, 0.0, 1.0);
        glLoadIdentity();
        glTranslated( (*curr)->getX(), (*curr)->getY(), (*curr)->getZ() );
        glScaled( ds, ds, ds );
        DrawSphere();
    }*/
};
