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
using namespace TooN;


class Environment {
public:
    Environment();
    virtual ~Environment();

    void setCamera( ATANCamera *c );
    ATANCamera* getCamera();

    void setCameraPose( SE3<> pose );
    SE3<> getCameraPose();

    void addPoint( Vector<3> point );
    std::vector< Vector<3> > &getPoints();

    void clearFeatures();
    void addFeature( Vector<3> feature );
    std::vector< Vector<3> > &getFeatures();
    /**
     * Answer the list of feature points that are within a given distance of a
     * vector. Assumes input vector at origin o and with formula given as o + (v-o)t
     */
    std::vector< Vector<3> > getFeatures( Vector<3> o, Vector<3> v, double tol );
    
private:
    ATANCamera* camera;
    SE3<> cameraPose;
    std::vector< Vector<3> > points;
    std::vector< Vector<3> > features;

};

#endif	/* _ENVIRONMENT_H */

