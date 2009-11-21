// -*- c++ -*-
// Copyright 2008 Isis Innovation Limited
//
// DropSphereGame.h
// Declares the DropSphereGame class
// DropSphereGame is a trivial AR app which draws some 3D graphics
// Draws a bunch of 3d eyeballs remniscient of the 
// AVL logo
//
#ifndef __DropSphereGame_H
#define __DropSphereGame_H
#include <TooN/TooN.h>
using namespace TooN;
#include <gvars3/GUI.h>
using namespace GVars3;
#include <vector>
#include "OpenGL.h"

class DropSphereGame {
public:
    DropSphereGame();
    virtual void DrawStuff(SE3<> camera);
    virtual void Reset();
    virtual void Init();
    static void create(void* obj, std::string cmd, std::string params);

protected:
    void DrawSphere();

    std::vector< Vector<3> > balls;
    bool initialised;
    SE3<> lastCam;

};


#endif
