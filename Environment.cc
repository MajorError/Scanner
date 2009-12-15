/* 
 * File:   Environment.cpp
 * Author: majorerror
 * 
 * Created on 17 November 2009, 14:59
 */

#include <algorithm>

#include "Environment.h"
#include "Point.h"
#include "PolyFace.h"

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

void Environment::setSceneSize( ImageRef size ) {
    sceneSize = size;
};

ImageRef& Environment::getSceneSize() {
    return sceneSize;
};

void Environment::addPoint( Point* point ) {
    points.push_back( point );
};

std::vector< Point* >& Environment::getPoints() {
    return points;
};

std::vector< Point* >& Environment::sortPoints( SE3<> camera ) {
    Matrix<> rot = camera.get_rotation().get_matrix();
    return sortPoints( camera.get_translation(), camera.get_translation()
            + makeVector( rot[0][2], rot[1][2], rot[2][2] ) );
};

std::vector< Point* >& Environment::sortPoints( Vector<3> o, Vector<3> v ) {
    Environment::o = o;
    Environment::v = v;
    std::sort( points.begin(), points.end(), Environment::closer );
    return points;
};

void Environment::addEdge( Point* from, Point* to ) {
    Edge* e = new Edge( from, to );
    edges.push_back( e );
    // Add the edge to the points
    from->addEdge( e );
    to->addEdge( e );
    unsigned int oldSize = 0;
    // Check if we've generated any new faces, and if so add to the set
    for( unsigned int k = 0; k < to->getEdges().size(); ++k ) {
        Edge* e2 = to->getEdges()[k];
        Point* mid = e2->getStart() == to ? e2->getEnd() : e2->getStart();
        for( unsigned int j = 0; j < mid->getEdges().size(); ++j ) {
            Edge* e3 = mid->getEdges()[j];
            Point* end = e3->getStart() == mid ? e3->getEnd() : e3->getStart();
            if ( end == from && end != mid && from != mid && to != mid && from != to ) {
                cerr << "Inserting new polyface...";
                oldSize = faces.size();
                faces.insert( new PolyFace( from, to, mid ) );
                cerr << (oldSize == faces.size() ? "skip" : "success") << endl;
            }
        }
    }
};

std::vector< Edge* > &Environment::getEdges() {
    return edges;
};

std::set< PolyFace*, PolyFace > &Environment::getFaces() {
    return faces;
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

bool Environment::closer( Point* a, Point* b ) {
    return closerVec( a->getPosition(), b->getPosition() );
};

bool Environment::closerVec( Vector<3> a, Vector<3> b ) {
    return MAG3( ((a - Environment::o) ^ (a - Environment::v)) )
            < MAG3( ((b - Environment::o) ^ (b - Environment::v)) );
};

std::vector< Vector<3> > Environment::getFeaturesSorted( SE3<> camera, double tol ) {
    Matrix<> rot = camera.get_rotation().get_matrix();
    Vector<3> view = makeVector( rot[0][2], rot[1][2], rot[2][2] );
    return getFeaturesSorted( camera.get_translation(), camera.get_translation() + view, tol );
};

std::vector< Vector<3> > Environment::getFeaturesSorted( Vector<3> o, Vector<3> v, double tol ) {
    Environment::v = v;
    Environment::o = o;
    std::vector< Vector<3> > out( getFeatures( o, v, tol ) );
    std::sort( out.begin(), out.end(), Environment::closerVec );
    return out;
};
