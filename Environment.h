/* 
 * File:   Environment.h
 * Author: majorerror
 *
 * Created on 17 November 2009, 14:59
 */

#ifndef _ENVIRONMENT_H
#define	_ENVIRONMENT_H

#include "ATANCamera.h"
#include "TooN/TooN.h"
#include "TooN/se3.h"
#include "cvd/image_ref.h"
#include "Point.h"
#include "PolyFace.h"
using namespace TooN;
using namespace std;
using namespace CVD;


class Environment {
public:
    Environment();
    virtual ~Environment();

    void setCamera( ATANCamera *c );
    ATANCamera* getCamera();

    void setCameraPose( SE3<> pose );
    SE3<> getCameraPose();

    void setSceneSize( ImageRef size );
    ImageRef& getSceneSize();

    void addPoint( Point* point );
    list< Point* > &getPoints();
    list< Point* > &sortPoints( SE3<> camera );
    list< Point* > &sortPoints( Vector<3> o, Vector<3> v );
    void removePoint( Point* p );

    void addEdge( Point* from, Point* to );
    void removeEdge( Edge* it );
    list< Edge* > &getEdges();
    Edge* findClosestEdge( Vector<3> &pointOnEdge, double& d );
    set< PolyFace*, PolyFace > &getFaces();
    
    void clearFeatures();
    void addFeature( Vector<3> feature );
    list< Vector<3> > &getFeatures();
    /**
     * Answer the list of feature points that are within a given distance of a
     * vector. Assumes input vector at origin o and with formula given as o + (v-o)t
     */
    list< Vector<3> > getFeatures( Vector<3> o, Vector<3> v, double tol );
    list< Vector<3> > getFeaturesSorted( SE3<> camera, double tol );
    list< Vector<3> > getFeaturesSorted( Vector<3> o, Vector<3> v, double tol );

    static Vector<3> v, o;
    static bool closer( Point* a, Point* b );
    static bool closerVec( Vector<3> a, Vector<3> b );
    
private:
    ATANCamera* camera;
    SE3<> cameraPose;
    ImageRef sceneSize;
    list< Point* > points;
    list< Edge* > edges;
    set< PolyFace*, PolyFace > faces;
    list< Vector<3> > features;

};

#endif	/* _ENVIRONMENT_H */

