/* 
 * File:   Environment.cpp
 * Author: majorerror
 * 
 * Created on 17 November 2009, 14:59
 */

#include <algorithm>

#include "Environment.h"

Vector<3> Environment::v;
Vector<3> Environment::o;

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
    //tol *= tol;
    for( unsigned int i = 0; i < features.size(); i++ ) {
        // o = x1, v = x2, feature = x0
        // d =	(|(x_0-x_1)x(x_0-x_2)|)/(|x_2-x_1|)
        d = MAG3( ((features[i] - o) ^ (features[i] - v)) ) / MAG3( (v - o) );
        if ( d <= tol )
            out.push_back( features[i] );
    }
    return out;
};

bool Environment::closer( Vector<3> a, Vector<3> b ) {
    return MAG3( ((a - Environment::o) ^ (a - Environment::v)) )
            < MAG3( ((b - Environment::o) ^ (b - Environment::v)) );
}

std::vector< Vector<3> > Environment::getFeaturesSorted( SE3<> camera, double tol ) {
    Matrix<> rot = camera.get_rotation().get_matrix();
    Vector<3> view = makeVector( rot[0][2], rot[1][2], rot[2][2] );
    return getFeaturesSorted( camera.get_translation(), camera.get_translation() + view, tol );
};

std::vector< Vector<3> > Environment::getFeaturesSorted( Vector<3> o, Vector<3> v, double tol ) {
    Environment::v = v;
    Environment::o = o;
    std::vector< Vector<3> > out( getFeatures( o, v, tol ) );
    std::sort( out.begin(), out.end(), Environment::closer );
    return out;
};
