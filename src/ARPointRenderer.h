// -*- c++ -*-
// Copyright 2008 Isis Innovation Limited
//
// ARPointRenderer.h
// Declares the ARPointRenderer class
// ARPointRenderer is a trivial piece of AR which draws some 3D graphics
//
#ifndef __ARPointRenderer_H
#define __ARPointRenderer_H

#include <TooN/TooN.h>
using namespace TooN;
#include <gvars3/instances.h>
using namespace GVars3;
#include <vector>
#include <map>
#include "Environment.h"
#include "OpenGL.h"

class ARPointRenderer {
public:
    ARPointRenderer( Environment *e );
    virtual void DrawStuff(SE3<> camera);

protected:
    void DrawPoints( SE3<> camera );
    void DrawPolys();
    void DrawFeatures( SE3<> camera );
    void DrawTarget( SE3<> camera );
    void DrawSphere();
    Environment *env;
    std::map< Image< Rgb< byte > >*, GLuint > textures;

};


#endif
