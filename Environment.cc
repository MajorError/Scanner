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

std::list< Point* >& Environment::getPoints() {
    return points;
};

std::list< Point* >& Environment::sortPoints( SE3<> camera ) {
    Matrix<> rot = camera.get_rotation().get_matrix();
    return sortPoints( camera.get_translation(), camera.get_translation()
            + makeVector( rot[0][2], rot[1][2], rot[2][2] ) );
};

std::list< Point* >& Environment::sortPoints( Vector<3> o, Vector<3> v ) {
    Environment::o = o;
    Environment::v = v;
    points.sort( Environment::closer );
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
    for( list<Edge*>::iterator curr = to->getEdges().begin();
            curr != to->getEdges().end(); curr++ ) {
        Point* mid = (*curr)->getStart() == to ? (*curr)->getEnd() : (*curr)->getStart();
        for( list<Edge*>::iterator curr2 = mid->getEdges().begin();
            curr2 != mid->getEdges().end(); curr2++ ) {
            Point* end = (*curr2)->getStart() == mid ? (*curr2)->getEnd() : (*curr2)->getStart();
            if ( end == from && end != mid && from != mid && to != mid && from != to ) {
                cerr << "Inserting new polyface...";
                oldSize = faces.size();
                faces.insert( new PolyFace( from, to, mid ) );
                cerr << (oldSize == faces.size() ? "skip" : "success") << endl;
            }
        }
    }
};

void Environment::removeEdge( Edge* e ) {
    edges.remove( e );
    cerr << "Culling faces" << endl;
    // Remove any faces containing this edge
    for( set<PolyFace*>::iterator curr = faces.begin();
            curr != faces.end(); curr++ ) {
        if( ((*curr)->getP1() == e->getStart()
                || (*curr)->getP2() == e->getStart()
                || (*curr)->getP3() == e->getStart())
            && ((*curr)->getP1() == e->getEnd()
                || (*curr)->getP2() == e->getEnd()
                || (*curr)->getP3() == e->getEnd()) ) {
            delete (*curr);
            faces.erase( curr );
        }
    }
    cerr << "Culling from start/end" << endl;
    e->getStart()->getEdges().remove( e );
    e->getEnd()->getEdges().remove( e );
    // Remove edge from start and end bundles
    /*list<Edge*> startEdges = (*it)->getStart()->getEdges();
    startEdges.remove( (*it) );
    (*it)->getStart()->getEdges() = startEdges;
    list<Edge*> endEdges = (*it)->getEnd()->getEdges();
    endEdges.remove( (*it) );
    (*it)->getEnd()->getEdges() = startEdges;*/
    /*for( list<Edge*>::iterator curr = e->getStart()->getEdges().begin();
            curr != e->getStart()->getEdges().end(); curr++ ) {
        cerr << "Examine " << (*curr) << endl;
        if( (*curr) == e ) {
            cerr << "\tERASE" << endl;
            curr = e->getStart()->getEdges().erase( curr );
        }
    }
    cerr << "Culling from end" << endl;
    for( list<Edge*>::iterator curr = e->getEnd()->getEdges().begin();
            curr != e->getEnd()->getEdges().end(); curr++ ) {
        if( (*curr) == e )
            curr = e->getEnd()->getEdges().erase( curr );
    }*/
    cerr << "Deleting pointer..." << endl;
    delete e;
};

std::list< Edge* > &Environment::getEdges() {
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

std::list< Vector<3> >& Environment::getFeatures() {
    return features;
};

#define MAG3(v) v[0] * v[0] + v[1] * v[1] + v[2] * v[2]
std::list< Vector<3> > Environment::getFeatures( Vector<3> o, Vector<3> v, double tol ) {
    double d; // the square of the distance
    std::list< Vector<3> > out;
    //tol *= tol;
    for( list< Vector<3> >::iterator curr = features.begin();
            curr != features.end(); curr++ ) {
        // o = x1, v = x2, feature = x0
        // d =	(|(x_0-x_1)x(x_0-x_2)|)/(|x_2-x_1|)
        d = MAG3( (((*curr) - o) ^ ((*curr) - v)) ) / MAG3( (v - o) );
        if ( d <= tol )
            out.push_back( (*curr) );
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

std::list< Vector<3> > Environment::getFeaturesSorted( SE3<> camera, double tol ) {
    Matrix<> rot = camera.get_rotation().get_matrix();
    Vector<3> view = makeVector( rot[0][2], rot[1][2], rot[2][2] );
    return getFeaturesSorted( camera.get_translation(), camera.get_translation() + view, tol );
};

std::list< Vector<3> > Environment::getFeaturesSorted( Vector<3> o, Vector<3> v, double tol ) {
    Environment::v = v;
    Environment::o = o;
    std::list< Vector<3> > out( getFeatures( o, v, tol ) );
    out.sort( Environment::closerVec );
    return out;
};
