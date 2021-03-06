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

#define PI 3.14159265
#define MAG3(v) v[0] * v[0] + v[1] * v[1] + v[2] * v[2]
Vector<3> Environment::v;
Vector<3> Environment::o;

Environment::Environment() {
    pthread_mutex_init( &mutex, NULL );
};

Environment::~Environment() {
    // no-op
};

void Environment::lock() {
    pthread_mutex_lock( &mutex );
};

void Environment::unlock() {
    pthread_mutex_unlock( &mutex );
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

void Environment::removePoint( Point* p ) {
    // Leave the work to the edge removal; this will cull faces too
    while( p->getEdges().size() > 0 )
        removeEdge( p->getEdges().front() );

    points.remove( p );

    delete p;
};

void Environment::addEdge( Point* from, Point* to ) {
    // Check if this edge already exists (in either direction)
    if ( from == to )
        return;
    for( list<Edge*>::iterator curr = edges.begin();
            curr != edges.end(); curr++ ) {
        if( ((*curr)->getStart() == from && (*curr)->getEnd() == to)
                || ((*curr)->getStart() == to && (*curr)->getEnd() == from) )
            return;
    }
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

Edge* Environment::findClosestEdge( Vector<3> &pointOnEdge, double& bestDistance ) {
    Edge* best = NULL;

    Vector<3> p0( cameraPose.get_translation() );
    Matrix<> rot = cameraPose.get_rotation().get_matrix();
    Vector<3> u = makeVector( rot[0][2], rot[1][2], rot[2][2] );

    // Now find the closest distance between L1 and L2, given that
    // L1 = p0 + mu0*u, and L2 = q0 + mu1*v
    // Based on http://softsurfer.com/Archive/algorithm_0106/algorithm_0106.htm
    for( list<Edge*>::iterator curr = edges.begin();
            curr != edges.end(); curr++ ) {
        Vector<3> q0( (*curr)->getStart()->getPosition() );
        Vector<3> v = (*curr)->getEnd()->getPosition() - q0;

        Vector<3> w0 = p0 - q0;
        double a = u * u;
        double b = u * v;
        double c = v * v;
        double d = u * w0;
        double e = v * w0;

        if ( (a*c - b*b) > 0 ) {
            // Calculate points
            Vector<3> edgePt = q0 + v * (a*e - b*d) / (a*c - b*b);
            Vector<3> targetPt = p0 + u * (b*e - c*d) / (a*c - b*b);
            // ... test if it's our best match yet
            Vector<3> res( targetPt - edgePt );
            double dist = MAG3( res );
            dist = dist < 0 ? -dist : dist;
            if ( dist < bestDistance ) {
                // Test to ensure that pointOnEdge would be actually ON edge
                double dp = v * (edgePt - q0);
                if ( dp >= 0 && dp < MAG3( v ) ) {
                    bestDistance = dist;
                    pointOnEdge = edgePt;
                    best = (*curr);
                }
            }
        }
    }
    return best;
}

void Environment::removeEdge( Edge* e ) {
    edges.remove( e );
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
    // Remove edge from start and end bundles
    e->getStart()->getEdges().remove( e );
    e->getEnd()->getEdges().remove( e );
    delete e;
};

std::list< Edge* > &Environment::getEdges() {
    return edges;
};

std::set< PolyFace*, PolyFace > &Environment::getFaces() {
    return faces;
};

PolyFace* Environment::findClosestFace( Vector<3> &pointOnFace ) {
    PolyFace* best = NULL;
    double bestDistance = numeric_limits<double>::max();

    Vector<3> p0( cameraPose.get_translation() );
    Matrix<> rot = cameraPose.get_rotation().get_matrix();
    Vector<3> u = makeVector( rot[0][2], rot[1][2], rot[2][2] );

    for( set<PolyFace*>::iterator curr = faces.begin(); curr != faces.end(); curr++ ) {
        // Find the point of intersection with view axis and face
        Vector<3> n = (*curr)->getFaceNormal();
        double td = n * u;
        if ( td == 0 ) // view axis is parallel to plane
            continue;
        double t = (n * ((*curr)->getP1()->getPosition() - p0)) / td;
        Vector<3> d = t * u;
        // Test distance to camera against bestDistance
        double distanceToCamera = MAG3( d );
        if ( distanceToCamera < bestDistance ) {
            // Test if intersection point is within PolyFace boundaries
            Vector<3> intersection = p0 + d;
            double xLower = min( (*curr)->getP1()->getPosition()[0], min( (*curr)->getP2()->getPosition()[0], (*curr)->getP3()->getPosition()[0] ) );
            double xUpper = max( (*curr)->getP1()->getPosition()[0], max( (*curr)->getP2()->getPosition()[0], (*curr)->getP3()->getPosition()[0] ) );
            double yLower = min( (*curr)->getP1()->getPosition()[1], min( (*curr)->getP2()->getPosition()[1], (*curr)->getP3()->getPosition()[1] ) );
            double yUpper = max( (*curr)->getP1()->getPosition()[1], max( (*curr)->getP2()->getPosition()[1], (*curr)->getP3()->getPosition()[1] ) );
            double zLower = min( (*curr)->getP1()->getPosition()[2], min( (*curr)->getP2()->getPosition()[2], (*curr)->getP3()->getPosition()[2] ) );
            double zUpper = max( (*curr)->getP1()->getPosition()[2], max( (*curr)->getP2()->getPosition()[2], (*curr)->getP3()->getPosition()[2] ) );
            if ( xLower < intersection[0] && xUpper > intersection[0]
                    && yLower < intersection[1] && yUpper > intersection[1]
                    && zLower < intersection[2] && zUpper > intersection[2] ) {
                best = (*curr);
                pointOnFace = intersection;
                bestDistance = distanceToCamera;
            }
        }
    }

    return best;
};

void Environment::findPlanarFaces( PolyFace* target, double tol, set<PolyFace*>& planarFaces ) {
    // Base case: we've already visited this face
    if ( target == NULL || planarFaces.count( target ) > 0 )
        return;
    // Test if this face has 0-area; two points will be at the same position
    if ( target->getP1()->getPosition() == target->getP2()->getPosition() ||
            target->getP1()->getPosition() == target->getP3()->getPosition() ||
            target->getP2()->getPosition() == target->getP3()->getPosition() ) {
        return;
    }
    // Add the face, search its siblings
    planarFaces.insert( target );
    Vector<3> n = target->getFaceNormal();
    for( set<PolyFace*>::iterator curr = faces.begin(); curr != faces.end(); curr++ ) {
        int ncp = 0;
        
        if ( (*curr)->getP1() == target->getP1()
                || (*curr)->getP1() == target->getP2()
                || (*curr)->getP1() == target->getP3() )
            ncp++;
        if ( (*curr)->getP2() == target->getP1()
                || (*curr)->getP2() == target->getP2()
                || (*curr)->getP2() == target->getP3() )
            ncp++;
        if ( (*curr)->getP3() == target->getP1()
                || (*curr)->getP3() == target->getP2()
                || (*curr)->getP3() == target->getP3() )
            ncp++;
        // If ncp > 1 we have a co-ocurrent face. Don't bother taking acos,
        //   since we can readily map our tolerance onto a graph of y = |cos(x)|
        double cosAlpha = n * (*curr)->getFaceNormal();
        if( ncp > 1 && 1 - (cosAlpha < 0 ? -cosAlpha : cosAlpha) < tol )
            findPlanarFaces( (*curr), tol, planarFaces );
    }
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

std::list< Vector<3> > Environment::getFeatures( Vector<3> o, Vector<3> v, double tol ) {
    double d; // the square of the distance
    std::list< Vector<3> > out;
    //tol *= tol;
    for( list< Vector<3> >::iterator curr = features.begin();
            curr != features.end(); curr++ ) {
        // o = x1, v = x2, feature = x0
        // d =	(|(x_0-x_1)x(x_0-x_2)|)/(|x_2-x_1|)
        Vector<3> a( ((*curr) - o) ^ ((*curr) - v) );
        Vector<3> b( v - o );
        d = MAG3( a ) / MAG3( b );
        if ( d <= tol )
            out.push_back( (*curr) );
    }
    return out;
};

bool Environment::closer( Point* a, Point* b ) {
    return closerVec( a->getPosition(), b->getPosition() );
};

bool Environment::closerVec( Vector<3> a, Vector<3> b ) {
    Vector<3> lt( (a - Environment::o) ^ (a - Environment::v) );
    Vector<3> gt( (b - Environment::o) ^ (b - Environment::v) );
    return MAG3( lt ) < MAG3( gt );
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
