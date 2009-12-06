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
    std::vector< Point* > &getPoints();
    std::vector< Point* > &sortPoints( SE3<> camera );
    std::vector< Point* > &sortPoints( Vector<3> o, Vector<3> v );

    void addEdge( Edge* edge );
    void addEdge( Point* from, Point* to );
    std::vector< Edge* > &getEdges();
    
    void clearFeatures();
    void addFeature( Vector<3> feature );
    std::vector< Vector<3> > &getFeatures();
    /**
     * Answer the list of feature points that are within a given distance of a
     * vector. Assumes input vector at origin o and with formula given as o + (v-o)t
     */
    std::vector< Vector<3> > getFeatures( Vector<3> o, Vector<3> v, double tol );
    std::vector< Vector<3> > getFeaturesSorted( SE3<> camera, double tol );
    std::vector< Vector<3> > getFeaturesSorted( Vector<3> o, Vector<3> v, double tol );

    static Vector<3> v, o;
    static bool closer( Point* a, Point* b );
    static bool closerVec( Vector<3> a, Vector<3> b );
    
private:
    ATANCamera* camera;
    SE3<> cameraPose;
    ImageRef sceneSize;
    std::vector< Point* > points;
    std::vector< Edge* > edges;
    std::vector< Vector<3> > features;

};

#endif	/* _ENVIRONMENT_H */

