// Copyright 2008 Isis Innovation Limited
#include "DropSphereGame.h"
#include "OpenGL.h"
#include <cvd/convolution.h>

using namespace CVD;

DropSphereGame::DropSphereGame() {
    mbInitialised = false;
}

void DropSphereGame::DrawStuff(Vector < 3 > v3CameraPos) {
    if (!mbInitialised)
        Init();

    mnFrameCounter++;

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

    for (int i = 0; i < 4; i++) {
        if (mnFrameCounter < 100)
            LookAt(i, 500.0 * (Vector < 3 >) makeVector((i < 2 ? -1 : 1)*(mnFrameCounter < 50 ? -1 : 1) * -0.4, -0.1, 1), 0.05);
        else
            LookAt(i, v3CameraPos, 0.02);

        glLoadIdentity();
        glMultMatrix(ase3WorldFromEye[i]);
        glScaled(mdEyeRadius, mdEyeRadius, mdEyeRadius);
        glCallList(mnEyeDisplayList);
    }

    glDisable(GL_LIGHTING);

    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, mnShadowTex);
    glEnable(GL_BLEND);
    glColor4f(0, 0, 0, 0.5);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2d(-mdShadowHalfSize, -mdShadowHalfSize);
    glTexCoord2f(0, 1);
    glVertex2d(-mdShadowHalfSize, mdShadowHalfSize);
    glTexCoord2f(1, 1);
    glVertex2d(mdShadowHalfSize, mdShadowHalfSize);
    glTexCoord2f(1, 0);
    glVertex2d(mdShadowHalfSize, -mdShadowHalfSize);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
};

void DropSphereGame::Reset() {
    mnFrameCounter = 0;

};

void DropSphereGame::Init() {
    if (mbInitialised)
        return;
    mbInitialised = true;
    // Set up the display list for the eyeball.
    mnEyeDisplayList = glGenLists(1);
    glNewList(mnEyeDisplayList, GL_COMPILE);
    DrawEye();
    glEndList();
    MakeShadowTex();
};




