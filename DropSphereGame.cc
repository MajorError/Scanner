// Copyright 2008 Isis Innovation Limited
#include "DropSphereGame.h"
#include "OpenGL.h"
#include <cvd/convolution.h>
#include <gvars3/gvars3.h>
#include <gvars3/instances.h>

using namespace CVD;
using namespace GVars3;

DropSphereGame::DropSphereGame() {
    initialised = false;
    balls.push_back( makeVector( 0.0, 0.0, 0.0 ) );
    balls.push_back( makeVector( 0.0, 1.0, 0.0 ) );
    balls.push_back( makeVector( 0.0, 0.0, 1.0 ) );
    balls.push_back( makeVector( 0.0, 1.0, 1.0 ) );

    GUI.RegisterCommand( "sphere.create", DropSphereGame::create, this );
}

void DropSphereGame::create(void* obj, std::string cmd, std::string params) {
    double d[3];
    std::stringstream paramStream( params );
    paramStream >> d[0];
    paramStream >> d[1];
    paramStream >> d[2];
    static_cast<DropSphereGame*>( obj )->balls.push_back( makeVector( d[0], d[1], d[2] ) );
}

void DropSphereGame::DrawStuff(Vector<3> v3CameraPos) {
    if (!initialised)
        Init();

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
    // Draw Spheres
    double ds = GV3::get<double>( "dotSize", 0.05 );
    for ( unsigned int i = 0; i < balls.size(); i++ ) {
        glLoadIdentity();
        glTranslate<3>( balls[i] );
        glScaled( ds, ds, ds );
        DrawSphere();
    }

    glDisable(GL_LIGHTING);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
};

void DropSphereGame::DrawSphere()
{
  int nSegments = 45;
  int nSlices = 45;

  double dSliceAngle = M_PI / (double)(nSlices);
  double dSegAngle = 2.0 * M_PI / (double)(nSegments);

  glColor4d(0.92, 0.9, 0.85,1);
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

void DropSphereGame::Reset() {
    // No-Op
};

void DropSphereGame::Init() {
    if (initialised)
        return;
    initialised = true;
};




