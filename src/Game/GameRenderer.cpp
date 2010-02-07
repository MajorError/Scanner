
#include "GameRenderer.h"
#include "AIUnit.h"
#include <vector>

GameRenderer::GameRenderer( WorldMap* m, Director* d, Environment* e ) : ARPointRenderer( e ), map( m ), director( d ) {
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

    DrawPolys();
    if ( GV3::get<bool>( "drawWaypoints", true ) )
        renderWaypointGraph();
    renderUnits();
    renderProjectiles();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
};

void GameRenderer::renderWaypointGraph() {
    double bs = GV3::get<double>( "waypointSize", 0.02 );
    for( list<Waypoint*>::iterator curr = map->getWaypoints().begin(); curr != map->getWaypoints().end(); curr++ ) {
        glColor4d( 1.0, 1.0, 0.0, 0.8 );
        glLoadIdentity();
        glTranslated( (*curr)->x, (*curr)->y, (*curr)->z );
        glScaled( bs, bs, bs );
        DrawSphere();
        glLoadIdentity();
        for( list<Waypoint*>::iterator goal = (*curr)->traversable.begin(); goal != (*curr)->traversable.end(); goal++ ) {
            glBegin( GL_LINES );
            glVertex3d( (*curr)->x, (*curr)->y, (*curr)->z );
            glVertex3d( (*goal)->x, (*goal)->y, (*goal)->z );
            glEnd();
        }
    }
};

void GameRenderer::renderUnits() {
    double ds = GV3::get<double>( "ptSize", 0.05 ) * 2;
    for( vector<AIUnit*>::iterator curr = director->getUnits().begin(); curr != director->getUnits().end(); curr++ ) {
        glColor4d( 0.4, 0.1, 1.0, 1.0 );
        glLoadIdentity();
        glTranslated( (*curr)->getX(), (*curr)->getY(), (*curr)->getZ() );
        glScaled( ds, ds, ds );
        DrawCube();
    }
};

void GameRenderer::renderProjectiles() {
    double ds = GV3::get<double>( "ptSize", 0.05 );
    for( vector<Projectile*>::iterator curr = director->getProjectiles().begin(); curr != director->getProjectiles().end(); curr++ ) {
        glColor4d( 1.0, 0.2, 0.0, 1.0 );
        glLoadIdentity();
        glTranslated( (*curr)->getX(), (*curr)->getY(), (*curr)->getZ() );
        glScaled( ds, ds, ds );
        DrawSphere();
    }
};

void GameRenderer::DrawCube() {
    glDisable( GL_CULL_FACE );
    glBegin( GL_QUADS );
    
    // Bottom
    glNormal3d( 0.0, -1.0, 0.0 );
    glVertex3d( -1.0, -1.0, -1.0 );
    glVertex3d( 1.0, -1.0, -1.0 );
    glVertex3d( 1.0, -1.0, 1.0 );
    glVertex3d( -1.0, -1.0, 1.0 );

    // Top
    glNormal3d( 0.0, 1.0, 0.0 );
    glVertex3d( 1.0, 1.0, 1.0 );
    glVertex3d( 1.0, -1.0, -1.0 );
    glVertex3d( -1.0, 1.0, -1.0 );
    glVertex3d( 1.0, 1.0, -1.0 );

    // Front
    glNormal3d( 0.0, 0.0, 1.0 );
    glVertex3d( -1.0, 1.0, 1.0 );
    glVertex3d( 1.0, 1.0, 1.0 );
    glVertex3d( 1.0, -1.0, 1.0 );
    glVertex3d( -1.0, -1.0, 1.0 );

    // Back
    glNormal3d( 0.0, 0.0, -1.0 );
    glVertex3d( -1.0, 1.0, -1.0 );
    glVertex3d( -1.0, -1.0, -1.0 );
    glVertex3d( 1.0, -1.0, -1.0 );
    glVertex3d( 1.0, 1.0, -1.0 );

    // Left
    glNormal3d( -1.0, 0.0, 0.0 );
    glVertex3d( -1.0, -1.0, -1.0 );
    glVertex3d( -1.0, 1.0, -1.0 );
    glVertex3d( -1.0, 1.0, 1.0 );
    glVertex3d( -1.0, -1.0, 1.0 );

    // Right
    glNormal3d( 1.0, 0.0, 0.0 );
    glVertex3d( 1.0, -1.0, -1.0 );
    glVertex3d( 1.0, -1.0, 1.0 );
    glVertex3d( 1.0, 1.0, 1.0 );
    glVertex3d( 1.0, 1.0, -1.0 );
    
    glEnd();
};
