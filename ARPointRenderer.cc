// Copyright 2008 Isis Innovation Limited
#include "ARPointRenderer.h"
#include "OpenGL.h"
#include "Environment.h"
#include <cvd/convolution.h>
#include <gvars3/gvars3.h>
#include <gvars3/instances.h>

using namespace CVD;
using namespace GVars3;

ARPointRenderer::ARPointRenderer( Environment *e ) : env( e ) {
}

void ARPointRenderer::DrawStuff(SE3<> camera) {
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

    if ( GV3::get<bool>( "drawPoints", true ) )
        DrawPoints( camera );
    if ( GV3::get<bool>( "drawModel", true ) )
        DrawPolys();
    if ( GV3::get<bool>( "drawFeatures", true ) )
        DrawFeatures( camera );
    if ( GV3::get<bool>( "drawTarget", true ) )
        DrawTarget( camera );
    

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
};

void ARPointRenderer::DrawPoints( SE3<> camera ) {
    double ds = GV3::get<double>( "ptSize", 0.05 );
    if ( GV3::get<bool>( "drawClosestPoint", true ) ) {
        glColor4d(1.0, 0.4, 0.0, 1.0);
        env->sortPoints( camera );
    }
    for ( list<Point*>::iterator curr = env->getPoints().begin();
            curr != env->getPoints().end(); curr++ ) {
        glLoadIdentity();
        glTranslate<3>( (*curr)->getPosition() );
        glScaled( ds, ds, ds );
        DrawSphere();
        glColor4d(0.92, 0.9, 0.85,1);
    }
};

void ARPointRenderer::DrawFeatures( SE3<> camera ) {
    double ds = GV3::get<double>( "ftSize", 0.01 );
    double rad = GV3::get<double>( "ftRadius", 1.0 );

    glColor4d(0.9, 0.2, 0.2, 1.0);
    std::list< Vector<3> > features( env->getFeaturesSorted( camera, rad ) );
    int num = 0;
    for ( list< Vector<3> >::iterator curr = features.begin();
            curr != features.end() && num < 5; curr++, num++ ) {
        glLoadIdentity();
        glTranslate<3>( *curr );
        glScaled( ds, ds, ds );
        DrawSphere();
        glColor4d(0.2, 0.2, 0.9, 0.8);
        ds *= 0.8;
    }
};

void ARPointRenderer::DrawTarget( SE3<> camera ) {
    double ds = GV3::get<double>( "ftSize", 0.01 );
    glLoadIdentity();
    glColor4d(0.2, 0.9, 0.2, 1.0);
    Matrix<> rot = camera.get_rotation().get_matrix();
    glTranslate<3>( camera.get_translation() + makeVector( rot[0][2], rot[1][2], rot[2][2] ) );
    glScaled( ds/2, ds/2, ds/2 );
    DrawSphere();
};

void ARPointRenderer::DrawPolys() {
    glLoadIdentity();
    glColor4d(0.2, 0.2, 0.9, 1.0);
    glBegin( GL_LINES );
    for( list<Edge*>::iterator curr = env->getEdges().begin();
            curr != env->getEdges().end(); curr++ ) {
        glVertex( (*curr)->getStart()->getPosition() );
        glVertex( (*curr)->getEnd()->getPosition() );
    }
    if ( GV3::get<bool>( "drawClosestEdge", true ) ) {
        glColor4d(1.0, 0.4, 0.0, 1.0);
        Vector<3> pt;
        double d = numeric_limits<double>::max();
        Edge* best = env->findClosestEdge( pt, d );
        if ( d < GV3::get<double>( "closestEdgeTol", 0.5 ) ) {
            glVertex( best->getStart()->getPosition() );
            glVertex( best->getEnd()->getPosition() );
        }
    }
    glEnd();

    glColor4d(1.0, 1.0, 1.0, 1.0);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_POLYGON_SMOOTH);
    
    glLoadIdentity();
    GLuint currTex;

    for( set<PolyFace*>::iterator it = env->getFaces().begin();
            it != env->getFaces().end(); it++ ) {
        // No empty entries or null pointers, please!
        if ( (*it) == NULL || (*it)->getP1() == NULL )
            continue;
        
        // Set up texture info
        glGenTextures( 1, &currTex );
        glPrintErrors();
        glBindTexture( GL_TEXTURE_2D, currTex );
        glPrintErrors();
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glPrintErrors();
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glPrintErrors();
        glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
        glPrintErrors();
        glTexImage2D( (*it)->getTexture(), 0, GL_TEXTURE_2D );
        glPrintErrors();
        
        glBegin( GL_TRIANGLES );

        // Render the face in both directions, so that it is visible from both
        //  sides (OpenGL will generate opposite normal vectors)
        /*cerr << (*it)->getP1()->getPosition() << " = " << (*it)->getP1Coord( env->getCamera() ) << ",\t";
        cerr << (*it)->getP2()->getPosition() << " = " << (*it)->getP2Coord( env->getCamera() ) << ",\t";
        cerr << (*it)->getP3()->getPosition() << " = " << (*it)->getP3Coord( env->getCamera() ) << endl;*/
        
        glTexCoord( (*it)->getP1Coord( env->getCamera() ) ); glVertex( (*it)->getP1()->getPosition() );
        glTexCoord( (*it)->getP2Coord( env->getCamera() ) ); glVertex( (*it)->getP2()->getPosition() );
        glTexCoord( (*it)->getP3Coord( env->getCamera() ) ); glVertex( (*it)->getP3()->getPosition() );

        glTexCoord( (*it)->getP3Coord( env->getCamera() ) ); glVertex( (*it)->getP3()->getPosition() );
        glTexCoord( (*it)->getP2Coord( env->getCamera() ) ); glVertex( (*it)->getP2()->getPosition() );
        glTexCoord( (*it)->getP1Coord( env->getCamera() ) ); glVertex( (*it)->getP1()->getPosition() );

        glEnd();
        glDeleteTextures( 1, &currTex );
        glPrintErrors();
    }
    glDisable(GL_TEXTURE_RECTANGLE_ARB);
};

void ARPointRenderer::DrawSphere()
{
  int nSegments = 45;
  int nSlices = 45;

  double dSliceAngle = M_PI / (double)(nSlices);
  double dSegAngle = 2.0 * M_PI / (double)(nSegments);

  {  // North pole:
    double Z = sin(M_PI/2.0 - dSliceAngle);
    double R = cos(M_PI/2.0 - dSliceAngle);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0,0,1);
    glVertex3f(0,0,1);
    for(int i=0; i<nSegments;i++)
      {
	glNormal3f(R * sin((double)i * dSegAngle), R * cos((double)i * dSegAngle),  Z);
	glVertex3f(R * sin((double)i * dSegAngle), R * cos((double)i * dSegAngle),  Z);
      }
    glNormal3f(0,R,Z);
    glVertex3f(0,R,Z);
    glEnd();
  }

  for(int j = 1; j<nSlices;j++)
    {

      glBegin(GL_QUAD_STRIP);
      double zTop = sin(M_PI/2.0 - dSliceAngle * (double)j);
      double zBot = sin(M_PI/2.0 - dSliceAngle * (double)(j+1));
      double rTop = cos(M_PI/2.0 - dSliceAngle * (double)j);
      double rBot = cos(M_PI/2.0 - dSliceAngle * (double)(j+1));
      for(int i=0; i<nSegments;i++)
	{
	  glNormal3f(rTop*sin((double)i*dSegAngle), rTop*cos((double)i*dSegAngle), zTop);
	  glVertex3f(rTop*sin((double)i*dSegAngle), rTop*cos((double)i*dSegAngle), zTop);
	  glNormal3f(rBot*sin((double)i*dSegAngle), rBot*cos((double)i*dSegAngle), zBot);
	  glVertex3f(rBot*sin((double)i*dSegAngle), rBot*cos((double)i*dSegAngle), zBot);
	};
      glNormal3f(0,rTop, zTop);
      glVertex3f(0,rTop, zTop);
      glNormal3f(0,rBot, zBot);
      glVertex3f(0,rBot, zBot);
      glEnd();
    };

  {
    // South pole:
    double Z = sin(M_PI/2.0 - dSliceAngle);
    double R = cos(M_PI/2.0 - dSliceAngle);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0,0,-1);
    glVertex3f(0,0,-1);
    for(int i=0; i<nSegments;i++)
      {
	glNormal3f(R * sin((double)i * -dSegAngle), R * cos((double)i * -dSegAngle),  -Z);
	glVertex3f(R * sin((double)i * -dSegAngle), R * cos((double)i * -dSegAngle),  -Z);
      }
    glNormal3f(0,R,-Z);
    glVertex3f(0,R,-Z);
    glEnd();
  };
};




