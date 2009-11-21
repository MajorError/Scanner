/* 
 * File:   Environment.cpp
 * Author: majorerror
 * 
 * Created on 17 November 2009, 14:59
 */

#include "Environment.h"

Environment::Environment() {
    // no-op
};

Environment::~Environment() {
    // no-op
};

void Environment::setCamera( ATANCamera *c ) {
    camera = c;
};

ATANCamera* Environment::getCamera() {
    return camera;
};

void Environment::setCameraPose( SE3<> pose ) {
    cameraPose = pose;
};

SE3<> Environment::getCameraPose() {
    return cameraPose;
};

void Environment::addPoint( Vector<3> point ) {
    points.push_back( point );
};

std::vector< Vector<3> >& Environment::getPoints() {
    return points;
};

void Environment::clearFeatures() {
    features.clear();
};

void Environment::addFeature( Vector<3> feature ) {
    features.push_back( feature );
};

std::vector< Vector<3> >& Environment::getFeatures() {
    return features;
};

#define MAG3(v) v[0] * v[0] + v[1] * v[1] + v[2] * v[2]
std::vector< Vector<3> > Environment::getFeatures( Vector<3> o, Vector<3> v, double tol ) {
    double d; // the square of the distance
    std::vector< Vector<3> > out;
    tol *= tol;
    for( unsigned int i = 0; i < features.size(); i++ ) {
        // o = x1, v = x2, feature = x0
        // d =	(|(x_0-x_1)x(x_0-x_2)|)/(|x_2-x_1|)
        d = MAG3( ((features[i] - o) ^ (features[i] - v)) ) / MAG3( (v - o) );
        if ( d <= tol )
            out.push_back( features[i] );
    }
    return out;
};
