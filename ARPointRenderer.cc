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
    initialised = false;
}

void ARPointRenderer::DrawStuff(SE3<> camera) {
    if (!initialised)
        Init();
    lastCam = camera;

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);

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
    // Draw Points
    double ds = GV3::get<double>( "ptSize", 0.05 );
    glColor4d(0.92, 0.9, 0.85,1);
    for ( unsigned int i = 0; i < env->getPoints().size(); i++ ) {
        glLoadIdentity();
        glTranslate<3>( env->getPoints()[i] );
        glScaled( ds, ds, ds );
        DrawSphere();
    }
    // Draw Features
    ds = GV3::get<double>( "ftSize", 0.03 );
    glColor4d(0.2, 0.2, 0.9,1);
    Matrix<> rot = camera.get_rotation().get_matrix();
    Vector<3> z = makeVector( rot[2][0], rot[2][1], rot[2][2] );
    std::vector< Vector<3> > features( env->getFeatures( camera.get_translation(), z, 1.0 ) );
    for ( unsigned int i = 0; i < features.size(); i++ ) {
        glLoadIdentity();
        glTranslate<3>( features[i] );
        glScaled( ds, ds, ds );
        DrawSphere();
    }

    // Draw a target ball
    /*std::cout << "Camera: t"
        << lastCam.get_translation() << std::endl
        << lastCam.get_rotation() << std::endl;
    Matrix<3,3> m( lastCam.get_rotation().get_matrix() );
    glLoadIdentity();
    glColor4d(0.2, 0.9, 0.2, 0.4);
    glTranslate<3>( makeVector( -m[2][0], -m[2][1], -m[2][2] ) );
    glScaled( ds/2, ds/2, ds/2 );
    DrawSphere();*/

    glDisable(GL_LIGHTING);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
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
      /*if(j == nBlueSlice)
	glColor3f(0,0,1);
      if(j == nWhiteSlice)*/

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
}

void ARPointRenderer::Reset() {
    // No-Op
};

void ARPointRenderer::Init() {
    if (initialised)
        return;
    initialised = true;
};




