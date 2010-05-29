#include <GL/gl.h>
#include <fstream>
#include <bits/stl_vector.h>
#include <cvd/image_io.h>

#include "Point.h"
#include "Environment.h"
#include "GUICommand.h"
#include "ARDriver.h"
#include "Game/GameRenderer.h"
#include "Game/WorldMap.h"
#include "Game/GameFactory.h"

#define PI 3.14159265

MK_GUI_COMMAND(key, handle,)
void key::handle( string s ) {
    for( unsigned int i = 0; i < Tool::list.size(); i++ ) {
        if ( Tool::list[i]->getHotKey() == s )
            Tool::list[i]->processClick();
    }
    // Chain all commands through to super KeyPress method (from PTAM)
    commandList::exec( "KeyPress "+s );
}

MK_GUI_COMMAND(edgelist, dump,)
void edgelist::dump( string s ) {
    cerr << "Edge list:\n" << endl;
    for( std::list<Edge*>::iterator curr = environment->getEdges().begin();
            curr != environment->getEdges().end(); curr++ ) {
        cerr << '\t' << (*curr)->getStart() << " -> " << (*curr)->getEnd() << endl;
    }
}

MK_GUI_COMMAND(pointlist, dump,)
void pointlist::dump( string s ) {
    cerr << "Point list:\n" << endl;
    for( std::list<Point*>::iterator curr = environment->getPoints().begin();
            curr != environment->getPoints().end(); curr++ ) {
        cerr << '\t' << (*curr) << " = " << (*curr)->getPosition() << endl;
    }
}


MK_GUI_COMMAND(polylist, dump,)
void polylist::dump( string s ) {
    cerr << "Poly list:\n" << endl;
    for( std::set<PolyFace*>::iterator curr = environment->getFaces().begin();
            curr != environment->getFaces().end(); curr++ ) {
        cerr << '\t' << (*curr) << " = " << (*curr)->getP1() << ", "
                << (*curr)->getP2() << ", " << (*curr)->getP3() << endl;
    }
}

namespace vtx1 {
    MK_GUI_COMMAND(vertex, create,)
    void vertex::create( string params ) {
        Vector<3> v;
        if ( params.size() > 0 ) {
            double d[3];
            std::stringstream paramStream( params );
            paramStream >> d[0];
            paramStream >> d[1];
            paramStream >> d[2];
            v = makeVector( d[0], d[1], d[2] );
        } else {
            double rad = GV3::get<double>( "ftRadius", 1.0 );
            std::list< Vector<3> > features( environment->getFeaturesSorted( environment->getCameraPose(), rad ) );
            if ( features.size() < 1 )
                return;
            v = features.front();
            cout << "vertex.create " << v[0] << ' ' << v[1] << ' ' << v[2] << endl;
        }
        environment->addPoint( new Point( v ) );
    }
}

namespace vtx2 {
    // Params used to "lock" part of the motion, by setting that axis to 0
    MK_GUI_COMMAND(vertex, move, SE3<> start; Vector<3> lock; pthread_t mover; static void* moveProcessor( void* ptr );)
    void vertex::move( string params ) {
        if ( environment->getPoints().size() == 0 )
            return;
        if ( !init ) {
            if ( params.length() > 0 ) {
                stringstream s( params );
                s >> lock;
            } else {
                lock = makeVector( 1, 1, 1 );
            }
            start = environment->getCameraPose();
            init = true;
            pthread_create( &mover, NULL, vertex::moveProcessor, (void*)this );
        } else {
            init = false;
            pthread_join( mover, NULL );
        }
    }

    void* vertex::moveProcessor( void* ptr ) {
        vertex *p = static_cast<vertex*>( ptr );
        Vector<3> projection;
        SE3<> camera( p->environment->getCameraPose() );
        Point* target = p->environment->sortPoints( camera ).front();
        // start point on list ~ camera + view*t
        Matrix<> rot = camera.get_rotation().get_matrix();
        projection = target->getPosition();
        projection -= camera.get_translation();
        projection[0] /= rot[0][2];
        projection[1] /= rot[1][2];
        projection[2] /= rot[2][2];
        Vector<3> lerp = makeVector( (1 - p->lock[0]) * target->getPosition()[0],
                (1 - p->lock[1]) * target->getPosition()[1], (1 - p->lock[2]) * target->getPosition()[2] );
        while( p->init ) {
            camera = p->environment->getCameraPose();
            rot = camera.get_rotation().get_matrix();
            // Now project as camera + view * startPt
            // Calculate in a separate list to prevent flickering
            // Lerp with locked version from start frame
            Vector<3> tmp;
            tmp[0] = p->lock[0] * (camera.get_translation()[0] + rot[0][2] * projection[0]) + lerp[0];
            tmp[1] = p->lock[1] * (camera.get_translation()[1] + rot[1][2] * projection[1]) + lerp[1];
            tmp[2] = p->lock[2] * (camera.get_translation()[2] + rot[2][2] * projection[2]) + lerp[2];
            target->setPosition( tmp );
        }
        return NULL;
    }
}

namespace vtx3 {
    MK_GUI_COMMAND(vertex, remove,)
    void vertex::remove( string params ) {
        if ( environment->getPoints().size() < 1 )
             return;
        Point* p = environment->sortPoints( environment->getCameraPose() ).front();
        environment->removePoint( p );
    }
}

namespace edge1 {
    MK_GUI_COMMAND(edge, connect, Point* from; )
    void edge::connect( string params ) {
        if ( environment->getPoints().size() < 2 )
             return;
        if ( params.length() > 2 ) {
            stringstream s( params );
            int from = -1;
            int to = -1;
            s >> from >> to;
            Point* fromPt = NULL;
            Point* toPt = NULL;
            int i = 0;
            for( std::list<Point*>::iterator curr = environment->getPoints().begin();
                    curr != environment->getPoints().end() && (i <= from || i <= to); curr++, i++ ) {
                if ( i == from )
                    fromPt = (*curr);
                if ( i == to )
                    toPt = (*curr);
            }
            if( fromPt != NULL && toPt != NULL )
                environment->addEdge( fromPt, toPt );
            return;
        }
        if ( !init ) {
            cerr << "edge.connect A" << endl;
            from = environment->sortPoints( environment->getCameraPose() ).front();
            init = true;
        } else {
            cerr << "edge.connect B" << endl;
            Point* to = environment->sortPoints( environment->getCameraPose() ).front();
            environment->addEdge( from, to );
            init = false;
        }
    }
}

namespace edge2 {
    MK_GUI_COMMAND(edge, bisect, )
    void edge::bisect( string params ) {
        Vector<3> bisection = makeVector( 0, 0, 0 );
        double d = numeric_limits<double>::max();
        Edge* target;
        if ( params.length() > 0 ) {
            stringstream paramStream( params );
            int idx = -1;
            paramStream >> idx >> bisection;
            if ( idx < 0 || idx > static_cast<int>( environment->getEdges().size() ) ) {
                cerr << "Invalid index, " << idx << " at " << bisection << endl;
                return;
            }
            cerr << "Using index " << idx << " at " << bisection << endl;
            target = *(environment->getEdges().begin().operator ++(idx));
        } else {
            target = environment->findClosestEdge( bisection, d );
            if ( target == NULL || d > GV3::get<double>( "edgeTolerance", 0.5 ) )
                return;
        }

        // if we found a valid target, bisect it
        Point* mid = new Point( bisection );
        Point* from = target->getStart();
        Point* to = target->getEnd();

        environment->removeEdge( target );
         // target = NULL; has been deleted

        environment->addPoint( mid );
        environment->addEdge( from, mid );
        environment->addEdge( mid, to );

        // If there's a polyface, re-connect 'mid' to third point
        // Only connect to points that are in *both* edge-sets
        for( std::list<Edge*>::iterator curr = from->getEdges().begin();
                curr != from->getEdges().end(); curr++ ) {
            Point* p1 = (*curr)->getStart() == from ? (*curr)->getEnd() : (*curr)->getStart();
            for( std::list<Edge*>::iterator inner = to->getEdges().begin();
                    inner != to->getEdges().end(); inner++ ) {
                if ( ((*inner)->getStart() == to ?
                    (*inner)->getEnd() : (*inner)->getStart()) == p1 )
                    environment->addEdge( mid, p1 );
            }
        }
    }
}

namespace edge3 {
    MK_GUI_COMMAND(edge, remove, )
    void edge::remove( string params ) {
        Vector<3> bisection = makeVector( 0, 0, 0 );
        double d = numeric_limits<double>::max();
        Edge* target = environment->findClosestEdge( bisection, d );
        if ( target == NULL || d > GV3::get<double>( "edgeTolerance", 0.5 ) )
            return;

        environment->removeEdge( target );
    }
}

namespace plane1 {
    MK_GUI_COMMAND(plane, move, SE3<> start; pthread_t mover; static void* moveProcessor( void* ptr );)
    void plane::move( string params ) {
        if ( !init ) {
            start = environment->getCameraPose();
            init = true;
            pthread_create( &mover, NULL, plane::moveProcessor, (void*)this );
        } else if ( environment->getPoints().size() > 0 ) {
            init = false;
            pthread_join( mover, NULL );
        }
    }

    void* plane::moveProcessor( void* ptr ) {
        plane *p = static_cast<plane*>( ptr );
        SE3<> camera( p->environment->getCameraPose() );

        Vector<3> pointOnFace;
        PolyFace* startTarget = p->environment->findClosestFace( pointOnFace );
        
        // start point on list ~ camera + view*t
        Matrix<> rot = camera.get_rotation().get_matrix();
        Vector<3> projection = pointOnFace;
        projection -= camera.get_translation();
        projection[0] /= rot[0][2];
        projection[1] /= rot[1][2];
        projection[2] /= rot[2][2];

        // Populate a set of the faces and points contained in the current plane
        set<PolyFace*> targets;
        map< Point*, Vector<3> > targetPoints;
        p->environment->findPlanarFaces( startTarget, GV3::get<double>( "planeTolerance", 0.1 ), targets );
        for( set<PolyFace*>::iterator curr = targets.begin(); curr != targets.end(); curr++ ) {
            targetPoints[(*curr)->getP1()] = (*curr)->getP1()->getPosition() - pointOnFace;
            targetPoints[(*curr)->getP2()] = (*curr)->getP2()->getPosition() - pointOnFace;
            targetPoints[(*curr)->getP3()] = (*curr)->getP3()->getPosition() - pointOnFace;
        }

        while( p->init ) {
            camera = p->environment->getCameraPose();
            rot = camera.get_rotation().get_matrix();
            // Now project as camera + view * startPt
            // Calculate in a separate list to prevent flickering
            Vector<3> tmp( camera.get_translation() );
            tmp[0] += rot[0][2] * projection[0];
            tmp[1] += rot[1][2] * projection[1];
            tmp[2] += rot[2][2] * projection[2];

            // Now tmp contains the vector of new pointOnFace; apply it to
            //   each of the points on each of the faces
            for( map< Point*, Vector<3> >::iterator curr = targetPoints.begin(); curr != targetPoints.end(); curr++ ) {
                curr->first->setPosition( curr->second + tmp );
            }
        }
        return NULL;
    }
}

namespace plane2 {
    MK_GUI_COMMAND(plane, extrude,)
    void plane::extrude( string params ) {
        Vector<3> pointOnFace;
        PolyFace* target = environment->findClosestFace( pointOnFace );

        // Populate a set of the faces contained in the current plane
        set<PolyFace*> targets;
        environment->findPlanarFaces( target, GV3::get<double>( "planeTolerance", 0.1 ), targets );

        cerr << "Extrude " << targets.size() << " faces.";

        map<Point*, Point*> pointMap; // Mapping from old -> new point
        Point* p;
        // Duplicate points
        for( set<PolyFace*>::iterator curr = targets.begin(); curr != targets.end(); curr++ ) {
            if ( pointMap.count( (*curr)->getP1() ) < 1 ) {
                p = new Point( (*curr)->getP1()->getPosition() );
                environment->addPoint( p );
                pointMap[(*curr)->getP1()] = p;
            }
            if ( pointMap.count( (*curr)->getP2() ) < 1 ) {
                p = new Point( (*curr)->getP2()->getPosition() );
                environment->addPoint( p );
                pointMap[(*curr)->getP2()] = p;
            }
            if ( pointMap.count( (*curr)->getP3() ) < 1 ) {
                p = new Point( (*curr)->getP3()->getPosition() );
                environment->addPoint( p );
                pointMap[(*curr)->getP3()] = p;
            }
        }
        cerr << " Duplicated " << pointMap.size() << " points." << endl;;

        // Connect together new plane's points
        for( map<Point*, Point*>::iterator curr = pointMap.begin(); curr != pointMap.end(); curr++ ) {
            environment->addEdge( curr->second, curr->first );
            for( std::list<Edge*>::iterator e = curr->first->getEdges().begin(); e  != curr->first->getEdges().end(); e++ ) {
                // Find the other end of this edge
                p = (*e)->getStart() == curr->first ? (*e)->getEnd() : (*e)->getStart();
                // If the end is in the pointMap, we should connect
                // Connection should duplicate existing faces, but also then
                //  construct polys between source and extruded plane
                if ( pointMap.count( p ) > 0 ) {
                    environment->addEdge( curr->second, pointMap[p] );

                    // Test if p already has a connection to the new plane
                    bool stop = false;
                    for( std::list<Edge*>::iterator pe = p->getEdges().begin(); !stop && pe != p->getEdges().end(); pe++ ) {
                        stop = ((*pe)->getStart() == p ? (*pe)->getEnd() : (*pe)->getStart()) == pointMap[p];
                    }
                    if ( stop )
                        continue;
                    
                    // Test if the current edge is on the plane boundary
                    int edgeCount = 0;
                    for( set<PolyFace*>::iterator f = targets.begin(); edgeCount < 2 && f != targets.end(); f++ ) {
                        int edgePtCount = 0;
                        if ( ((*f)->getP1() == p || (*f)->getP1() == curr->first) )
                            edgePtCount++;
                        if ( ((*f)->getP2() == p || (*f)->getP2() == curr->first) )
                            edgePtCount++;
                        if ( ((*f)->getP3() == p || (*f)->getP3() == curr->first) )
                            edgePtCount++;
                        if ( edgePtCount > 1 )
                            edgeCount++;
                    }
                    // If so, connect the new face to the old face
                    if ( edgeCount <= 1 ) {
                        environment->addEdge( curr->second, p );
                    }
                }
            }
        }
    }
}

namespace plane3 {
    MK_GUI_COMMAND(plane, split,)
    void plane::split( string params ) {
        Vector<3> pointPos;
        PolyFace* target = environment->findClosestFace( pointPos );
        if ( target == NULL )
            return;
        
        Point* p = new Point( pointPos );
        environment->addPoint( p );

        environment->addEdge( target->getP1(), p );
        environment->addEdge( target->getP2(), p );
        environment->addEdge( target->getP3(), p );
        
        environment->getFaces().erase( target );
        delete target;
    }
}

namespace plane4 {
    MK_GUI_COMMAND(plane, revolve,)
    void plane::revolve( string params ) {
        double d = numeric_limits<double>::max();
        Vector<3> p;
        Edge* axisEdge = environment->findClosestEdge( p, d );
        // Generate a normalised axis
        Vector<3> axis = axisEdge->getStart()->getPosition() - axisEdge->getEnd()->getPosition();
        axis /= sqrt( axis * axis );
        PolyFace* targetFace = NULL;
        for( set<PolyFace*>::iterator curr = environment->getFaces().begin();
                curr != environment->getFaces().end(); curr++ ) {
            int nf = 0;
            if ( (*curr)->getP1() == axisEdge->getStart() || (*curr)->getP1() == axisEdge->getEnd() )
                nf++;
            if ( (*curr)->getP2() == axisEdge->getStart() || (*curr)->getP2() == axisEdge->getEnd() )
                nf++;
            if ( (*curr)->getP3() == axisEdge->getStart() || (*curr)->getP3() == axisEdge->getEnd() )
                nf++;
            if ( nf >= 2 ) {
                targetFace = *curr;
                break;
            }
        }
        if ( targetFace == NULL )
            return;
        set<PolyFace*> planeTemplate;
        environment->findPlanarFaces( targetFace, GV3::get<double>( "planeTolerance", 0.1 ), planeTemplate );
        
        double numFaces = GV3::get<double>( "revolveFaces", 25 );
        double freq = (PI * 2) / numFaces;
        Matrix<3,3> transToOrigin;
        transToOrigin[0] = makeVector( 1, 0, 0 );
        transToOrigin[1] = makeVector( 0, 1, 0 );
        transToOrigin[2] = -axisEdge->getStart()->getPosition();
        SO3<> rotToAxis( axis, makeVector( 1, 0, 0 ) );
        SO3<> rotBack = rotToAxis.inverse();
        Matrix<3,3> transBack;
        transBack[0] = makeVector( 1, 0, 0 );
        transBack[1] = makeVector( 0, 1, 0 );
        transBack[2] = axisEdge->getStart()->getPosition();
        map< Point*, vector<Point*> > pointProjection;
        set<Point*> visited;
        for( set<PolyFace*>::iterator fc = planeTemplate.begin(); fc != planeTemplate.end(); fc++ ) {
            if ( visited.count( (*fc)->getP1() ) == 0 ) {
                pointProjection[(*fc)->getP1()].push_back( (*fc)->getP1() );
                visited.insert( (*fc)->getP1() );
            }
            if ( visited.count( (*fc)->getP2() ) == 0 ) {
                pointProjection[(*fc)->getP2()].push_back( (*fc)->getP2() );
                visited.insert( (*fc)->getP2() );
            }
            if ( visited.count( (*fc)->getP3() ) == 0 ) {
                pointProjection[(*fc)->getP3()].push_back( (*fc)->getP3() );
                visited.insert( (*fc)->getP3() );
            }
        }
        for ( double i = 1; i <= numFaces; i++ ) {
            double theta = i * freq;
            double sinTheta = sin( theta );
            double cosTheta = cos( theta );

            Matrix<3,3> rot;
            rot[0] = makeVector( 1, 0, 0 );
            rot[1] = makeVector( 0, cosTheta, sinTheta  );
            rot[2] = makeVector( 0, -sinTheta, cosTheta );
            Matrix<3,3> xform = rotToAxis.get_matrix() * rot * rotBack.get_matrix();
            
            // Ensure there is no duplication of points
            map<Point*,Point*> pointMap;
            
            for( set<PolyFace*>::iterator fc = planeTemplate.begin(); fc != planeTemplate.end(); fc++ ) {
                // Each new point is (pos - axisStart) * xform + axisStart
                Point* p1 = NULL;
                if ( pointMap.count( (*fc)->getP1() ) == 0 &&
                        (*fc)->getP1() != axisEdge->getStart() && (*fc)->getP1() != axisEdge->getEnd() ) {
                    p1 = new Point( ((*fc)->getP1()->getPosition() - axisEdge->getStart()->getPosition())
                            * xform + axisEdge->getStart()->getPosition() );
                    pointMap[(*fc)->getP1()] = p1;
                    environment->addPoint( p1 );
                    pointProjection[(*fc)->getP1()].push_back( p1 );
                    if ( (*fc)->getP2() == axisEdge->getStart() || (*fc)->getP2() == axisEdge->getEnd() )
                        environment->addEdge( p1, (*fc)->getP2() );
                    if ( (*fc)->getP3() == axisEdge->getStart() || (*fc)->getP3() == axisEdge->getEnd() )
                        environment->addEdge( p1, (*fc)->getP3() );
                }
                Point* p2 = NULL;
                if ( pointMap.count( (*fc)->getP2() ) == 0 &&
                        (*fc)->getP2() != axisEdge->getStart() && (*fc)->getP2() != axisEdge->getEnd() ) {
                    p2 = new Point( ((*fc)->getP2()->getPosition() - axisEdge->getStart()->getPosition())
                            * xform + axisEdge->getStart()->getPosition() );
                    pointMap[(*fc)->getP2()] = p2;
                    environment->addPoint( p2 );
                    pointProjection[(*fc)->getP2()].push_back( p2 );
                    if ( (*fc)->getP1() == axisEdge->getStart() || (*fc)->getP1() == axisEdge->getEnd() )
                        environment->addEdge( p2, (*fc)->getP1() );
                    if ( (*fc)->getP3() == axisEdge->getStart() || (*fc)->getP3() == axisEdge->getEnd() )
                        environment->addEdge( p2, (*fc)->getP3() );
                }
                Point* p3 = NULL;
                if ( pointMap.count( (*fc)->getP3() ) == 0 &&
                        (*fc)->getP3() != axisEdge->getStart() && (*fc)->getP3() != axisEdge->getEnd() ) {
                    p3 = new Point( ((*fc)->getP3()->getPosition() - axisEdge->getStart()->getPosition())
                            * xform + axisEdge->getStart()->getPosition() );
                    pointMap[(*fc)->getP3()] = p3;
                    environment->addPoint( p3 );
                    pointProjection[(*fc)->getP3()].push_back( p3 );
                    if ( (*fc)->getP1() == axisEdge->getStart() || (*fc)->getP1() == axisEdge->getEnd() )
                        environment->addEdge( p3, (*fc)->getP1() );
                    if ( (*fc)->getP2() == axisEdge->getStart() || (*fc)->getP2() == axisEdge->getEnd() )
                        environment->addEdge( p3, (*fc)->getP2() );
                }
                environment->addEdge( pointMap.count( (*fc)->getP1() ) > 0 ? pointMap[(*fc)->getP1()] : (*fc)->getP1(),
                        pointMap.count( (*fc)->getP2() ) > 0 ? pointMap[(*fc)->getP2()] : (*fc)->getP2() );
                environment->addEdge( pointMap.count( (*fc)->getP1() ) > 0 ? pointMap[(*fc)->getP1()] : (*fc)->getP1(),
                        pointMap.count( (*fc)->getP2() ) > 0 ? pointMap[(*fc)->getP2()] : (*fc)->getP2() );
                environment->addEdge( pointMap.count( (*fc)->getP2() ) > 0 ? pointMap[(*fc)->getP2()] : (*fc)->getP2(),
                        pointMap.count( (*fc)->getP3() ) > 0 ? pointMap[(*fc)->getP3()] : (*fc)->getP3() );
            }
        }

        // We have to record the edges, not just create them, so we don't see
        //    the new edges on the next iteration
        multimap<Point*,Point*> edgeTodo;
        for( map< Point*, vector<Point*> >::iterator pointPair = pointProjection.begin();
                pointPair != pointProjection.end(); pointPair++ ) {
            Point* prev = pointPair->second.back();
            for( vector<Point*>::iterator curr = pointPair->second.begin(); curr != pointPair->second.end(); curr++ ) {
                // Connect to the previous projected point
                edgeTodo.insert( pair<Point*,Point*>( prev, *curr ) );
                // Now triangulate with other points on that face
                for( std::list<Edge*>::iterator e = prev->getEdges().begin(); e != prev->getEdges().end(); e++ ) {
                    // Connect to the other end of this edge
                    Point* p = (*e)->getStart() == *curr ? (*e)->getEnd() : (*e)->getStart();
                    edgeTodo.insert( pair<Point*,Point*>( *curr, p ) );
                }
                prev = *curr;
            }
        }
        for( multimap<Point*,Point*>::iterator pair = edgeTodo.begin();
                pair != edgeTodo.end(); pair++ ) {
            environment->addEdge( pair->first, pair->second );
        }
    }
}

namespace {
    MK_GUI_COMMAND(text, draw,)
    void text::draw( string text ) {
        pair<double, double> textSize = glGetExtends( text, 1.8, 0.2 );
        ImageRef textPos( 10, 25 );
        textSize.first *= 10;
        textSize.second *= 10;
        glMatrixMode( GL_PROJECTION );
        glPushMatrix();
        
        // Bounding Box
        glColor4d( 0.35, 0.5, 0.9, 1.0 );
        glBegin( GL_QUADS );
        glVertex3d( 8.0, 8.0, 1.0 );
        glVertex3d( textSize.first + 12, 8.0, 1.0 );
        glVertex3d( textSize.first + 12, textSize.second + 15, 1.0 );
        glVertex3d( 8.0, textSize.second + 15, 1.0 );
        glEnd();
        
        glColor4d( 0.5, 0.65, 1.0, 1.0 );
        glLineWidth( 1.5 );
        glBegin( GL_LINE_STRIP );
        glVertex3d( 8.0, 8.0, 1.0 );
        glVertex3d( textSize.first + 12, 8.0, 1.0 );
        glVertex3d( textSize.first + 12, textSize.second + 15, 1.0 );
        glVertex3d( 8.0, textSize.second + 15, 1.0 );
        glVertex3d( 8.0, 8.0, 1.0 );
        glEnd();
        
        // Text draw
        glTranslate( textPos );
        glScalef( 10, -10, 1 );
        glColor4d( 1.0, 1.0, 1.0, 1.0 );
        glDrawText( text, FILL, 1.8, 0.2 );
        glPopMatrix();
    }
}

namespace tex1 {
    MK_GUI_COMMAND(texture, clean,)
    void texture::clean( string args ) {
        double tol = GV3::get<double>( "texTolerance1", 1.0 );

        // Set common texture (by ref) based on camera distance
        for( set<PolyFace*>::iterator curr = environment->getFaces().begin();
                curr != environment->getFaces().end(); curr++ ) {
            // Look at all other texture frames; if it was taken from a
            //   sufficiently similar camera angle, use common image
            for( set<PolyFace*>::iterator inner = environment->getFaces().begin();
                    inner != environment->getFaces().end(); inner++ ) {
                Vector<3> v = (*inner)->getTextureViewpoint().get_translation() - (*curr)->getTextureViewpoint().get_translation();
                if ( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] < tol ) {
                    (*inner)->testBoundsAndSetTexture( &(*curr)->getTexture(),
                            (*curr)->getTextureViewpoint(), environment->getCamera() );
                }
            }
        }

        tol = GV3::get<double>( "texTolerance2", 3.0 );
        int ncp; // number of co-occurrent points
        std::list<PolyFace*> faces; // adjacent faces
        // Reduce the number of region boundaries as much as possible
        for( set<PolyFace*>::iterator curr = environment->getFaces().begin();
                curr != environment->getFaces().end(); curr++ ) {
            faces.clear();
            // Look at adjacent bits of texture to (*curr); if it was taken from a
            //   sufficiently similar camera angle, choose whichever image gives the
            //   most coverage
            for( set<PolyFace*>::iterator inner = environment->getFaces().begin();
                    inner != environment->getFaces().end(); inner++ ) {
                if ( (*inner)->getTextureSource() == (*curr)->getTextureSource() )
                    continue;
                ncp = 0;
                if ( (*curr)->getP1() == (*inner)->getP1()
                        || (*curr)->getP1() == (*inner)->getP2()
                        || (*curr)->getP1() == (*inner)->getP3() )
                    ncp++;
                if ( (*curr)->getP2() == (*inner)->getP1()
                        || (*curr)->getP2() == (*inner)->getP2()
                        || (*curr)->getP2() == (*inner)->getP3() )
                    ncp++;
                if ( (*curr)->getP3() == (*inner)->getP1()
                        || (*curr)->getP3() == (*inner)->getP2()
                        || (*curr)->getP3() == (*inner)->getP3() )
                    ncp++;
                // ncp > 1 -> co-occurrent edge
                if ( ncp > 1 )
                    faces.push_back( *inner );
            }
            // If we're bounded on at least 2 sides by the *same* texture, and
            //   we're within tolerance2, homogenise textures
            if ( faces.size() > 1 && faces.front()->getTextureSource() == (*(++faces.begin()))->getTextureSource() ) {
                Vector<3> v = faces.front()->getTextureViewpoint().get_translation() - (*curr)->getTextureViewpoint().get_translation();
                if ( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] < tol ) {
                    (*curr)->testBoundsAndSetTexture( &faces.front()->getTexture(),
                            faces.front()->getTextureViewpoint(), environment->getCamera() );
                }
            }
        }
    }
}

namespace tex2 {
    MK_GUI_COMMAND(texture, clear,)
    void texture::clear( string args ) {
        Vector<3> pt;
        PolyFace* f = environment->findClosestFace( pt );
        if ( f == NULL )
            return;
        Image< Rgb<byte> > i( ImageRef( 640, 480 ), Rgb<byte>( 255, 0, 0 ) );
        SE3<> trans = makeVector( numeric_limits<double>::max(),
                numeric_limits<double>::max(), numeric_limits<double>::max(),
                numeric_limits<double>::max(), numeric_limits<double>::max(),
                numeric_limits<double>::max() );
        f->setTexture( i, trans );
    }
}

#ifdef USE_EXPERIMENTALS
namespace {
    MK_GUI_COMMAND(textureBoundary, blend,)
    void textureBoundary::blend( string args ) {
        int ncp; // number of co-occurrent points
        for( set<PolyFace*>::iterator curr = environment->getFaces().begin();
                curr != environment->getFaces().end(); curr++ ) {
            // Look at all other texture frames, and find those which are adjacent
            //   and different
            for( set<PolyFace*>::iterator inner = environment->getFaces().begin();
                    inner != environment->getFaces().end(); inner++ ) {
                if ( (*inner)->getTextureSource() == (*curr)->getTextureSource() )
                    continue;
                ncp = 0;
                if ( (*curr)->getP1() == (*inner)->getP1()
                        || (*curr)->getP1() == (*inner)->getP2()
                        || (*curr)->getP1() == (*inner)->getP3() )
                    ncp++;
                if ( (*curr)->getP2() == (*inner)->getP1()
                        || (*curr)->getP2() == (*inner)->getP2()
                        || (*curr)->getP2() == (*inner)->getP3() )
                    ncp++;
                if ( (*curr)->getP3() == (*inner)->getP1()
                        || (*curr)->getP3() == (*inner)->getP2()
                        || (*curr)->getP3() == (*inner)->getP3() )
                    ncp++;
                // ncp > 1 -> co-occurrent edge, so perform blend
                if ( ncp > 1 ) {
                    // First, save the source texture viewpoints
                    SE3<> vp1 = (*curr)->getTextureViewpoint();
                    SE3<> vp2 = (*inner)->getTextureViewpoint();
                    // Now, calculate P1, P2, P3 for the CURRENT texture
                    Vector<2> currP1 = (*curr)->getP1Coord( environment->getCamera() );
                    Vector<2> currP2 = (*curr)->getP2Coord( environment->getCamera() );
                    Vector<2> currP3 = (*curr)->getP3Coord( environment->getCamera() );
                    Vector<2> innerP1 = (*inner)->getP1Coord( environment->getCamera() );
                    Vector<2> innerP2 = (*inner)->getP2Coord( environment->getCamera() );
                    Vector<2> innerP3 = (*inner)->getP3Coord( environment->getCamera() );
                    // Next, calculate P1, P2, P3 for the OTHER texture
                    (*curr)->setTexture( (*curr)->getTexture(), vp2 );
                    (*inner)->setTexture( (*inner)->getTexture(), vp1 );
                    Vector<2> currP1o = (*curr)->getP1Coord( environment->getCamera() );
                    Vector<2> currP2o = (*curr)->getP2Coord( environment->getCamera() );
                    Vector<2> currP3o = (*curr)->getP3Coord( environment->getCamera() );
                    Vector<2> innerP1o = (*inner)->getP1Coord( environment->getCamera() );
                    Vector<2> innerP2o = (*inner)->getP2Coord( environment->getCamera() );
                    Vector<2> innerP3o = (*inner)->getP3Coord( environment->getCamera() );
                    (*curr)->setTexture( (*curr)->getTexture(), vp1 );
                    (*inner)->setTexture( (*inner)->getTexture(), vp2 );
                    // Blend textures onto each other, simultaneously
                    Image< Rgb<byte> > ctOrig( (*curr)->getTexture().copy_from_me() );
                    Image< Rgb<byte> > itOrig( (*inner)->getTexture().copy_from_me() );
                    // Before we blend, work out how to map (x,y) on original to (x',y') on target
                    double amt = 0; // Lerp factor
                    int i0 = 0; // i projected on the first image
                    int j0 = 0; // j projected on the first image
                    int i1 = 0; // i projected on the other image
                    int j1 = 0; // j projected on the other image
                    for( int i = 0; i < ctOrig.size()[0]; i++ ) {
                        for( int j = 0; j < ctOrig.size()[1]; j++ ) {
                            // Work out (i0,j0) -> (i1,j1)
                            i0 = i;
                            j0 = j;
                            i1 = i;
                            j1 = j;
                            // lerp should be zero at and beyond the boundary
                            // should blend to 1 before the far corner of the triangle
                            amt = 1;
                            // Apply lerp
                            (*curr)->getTexture()[i0][j0] = amt * ctOrig[i0][j0] + (1 - amt) * itOrig[i1][j1];

                            // Same process, in reverse
                            i0 = i;
                            j0 = j;
                            i1 = i;
                            j1 = j;
                            amt = 1;
                            (*inner)->getTexture()[i0][j0] = amt * itOrig[i0][j0] + (1 - amt) * ctOrig[i1][j1];
                        }
                    }
                }
            }
        }
    }
}
#endif
#ifdef USE_EXPERIMENTALS
namespace {
    MK_GUI_COMMAND(shrinkwrap, exec,)
    void shrinkwrap::exec( string args ) {
        double tol = GV3::get<double>( "shrinkwrapTolerance", 0.001 );
        int idx = 0;
        
        std::list<int> edges;
        std::list< Vector<3> > features;
        for( std::list<Edge*>::iterator curr = environment->getEdges().begin();
                curr != environment->getEdges().end(); curr++, idx++ ) {
            cerr << "Examine " << *curr << " (" << idx << ")";
            // Get all features within the given tol distance to this vector
            std::list< Vector<3> > sorted = environment->getFeaturesSorted(
                    (*curr)->getStart()->getPosition(),
                    (*curr)->getEnd()->getPosition(), tol );
            cerr << ": Got " << sorted.size() << " features." << endl;
            Vector<3> vStart = (*curr)->getStart()->getPosition();
            Vector<3> v = (*curr)->getEnd()->getPosition() - vStart;
            for( std::list< Vector<3> >::iterator feat = sorted.begin();
                    feat != sorted.end(); feat++ ) {
                // Now check the the feature lies within the bounds of (*curr)
                        cerr << "Looking at " << *feat;
                if ( (v * ((*feat) - vStart)) >= 0 ) {
                    // As this is the case, bisect the current edge and create
                    //  a point on *feat
                    cerr << ": BISECT";
                    edges.push_back( idx );
                    features.push_back( (*feat) );
                }
                cerr << "... DONE!" << endl;
            }
        }
        std::list<int>::iterator currEdge = edges.begin();
        std::list< Vector<3> >::iterator currFeat = features.begin();
        int startPt = environment->getPoints().size();
        int startEdge = environment->getEdges().size();
        for( ; currEdge != edges.end() && currFeat != features.end(); currEdge++, currFeat++ ) {
            bool exec = true;
            // First, check if we already have a point close to the target
            for( std::list<Point*>::iterator currPt = environment->getPoints().begin();
                    exec && currPt != environment->getPoints().end(); currPt++ ) {
                Vector<3> v( (*currFeat) - (*currPt)->getPosition() );
                if ( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] < tol )
                    exec = false;
            }
            // Next, check if the point is close to the surface of an existing face
            for( std::set<PolyFace*>::iterator currFace = environment->getFaces().begin();
                    exec && currFace != environment->getFaces().end(); currFace++ ) {
                // Calculate unit normal vector
                if( (*currFace)->getFaceNormal() * ((*currFeat) - (*currFace)->getP1()->getPosition()) < tol )
                    exec = false;
            }
            if ( exec ) {
                stringstream cmdStream;
                cmdStream << "edge.bisect " << *currEdge << ' ' << *currFeat << endl;
                GUI.ParseLine( cmdStream.str() );
            }
        }
        cerr << "From " << startPt << " to " << environment->getPoints().size() << " points" << endl;
        cerr << "From " << startEdge << " to " << environment->getEdges().size() << " edges" << endl;
    }
}
#endif

namespace mesh1 {
    
    MK_GUI_COMMAND(mesh, subdivide,)
    void mesh::subdivide( string params ) {
        multimap<Point*, Point*> pts;
        multimap< Point*,Vector<3> > facePoints;
        std::list<Point*> oldPoints = environment->getPoints();
        // Collect the list of points to construct
        for( std::set<PolyFace*>::iterator curr = environment->getFaces().begin();
                curr != environment->getFaces().end(); curr++ ) {
            Point* p = new Point( (*curr)->getFaceCentre() );
            environment->addPoint( p );
            pts.insert( pair<Point*,Point*>( p, (*curr)->getP1() ) );
            pts.insert( pair<Point*,Point*>( p, (*curr)->getP2() ) );
            pts.insert( pair<Point*,Point*>( p, (*curr)->getP3() ) );
            // Record the facePoints for this face
            facePoints.insert( pair< Point*,Vector<3> >( (*curr)->getP1(), p->getPosition() ) );
            facePoints.insert( pair< Point*,Vector<3> >( (*curr)->getP2(), p->getPosition() ) );
            facePoints.insert( pair< Point*,Vector<3> >( (*curr)->getP3(), p->getPosition() ) );
            // Delete the current face; pointers get dropped after this loop
            delete *curr;
        }

        environment->getFaces().clear();
        
        // Catmull-clark averaging of point positions
        double smoothAmt = GV3::get<double>( "catmullSmooth", 0.2 );
        for( std::list<Point*>::iterator curr = oldPoints.begin(); curr != oldPoints.end(); curr++ ) {
            // Calculate edge-mid-point average
            Vector<3> edgeAvg = makeVector( 0, 0, 0 );
            int numEdges = 0;
            for( std::list<Edge*>::iterator it = (*curr)->getEdges().begin(); it != (*curr)->getEdges().end(); it++ ) {
                edgeAvg += 0.5 * ( (*it)->getStart()->getPosition() + (*it)->getEnd()->getPosition() );
                numEdges++;
            }
            edgeAvg /= numEdges;
            // Now calculate average face-centre for faces around curr
            Vector<3> faceAvg = makeVector( 0, 0, 0 );
            pair< multimap< Point*,Vector<3> >::iterator, multimap< Point*,Vector<3> >::iterator > range =
                    facePoints.equal_range( *curr );
            int numFaces = 0;
            for( multimap< Point*,Vector<3> >::iterator el = range.first; el != range.second; el++ ) {
                faceAvg += el->second;
                numFaces++;
            }
            faceAvg /= numFaces;
            (*curr)->setPosition( (1 - smoothAmt) * (*curr)->getPosition()
                    + smoothAmt * (faceAvg + 2*edgeAvg + (numEdges-3)*(*curr)->getPosition()) / numEdges );
        }

        // Now we've moved the control points, construct the new subdiv polys
        for( multimap<Point*, Point*>::iterator el = pts.begin(); el != pts.end(); el++ ) {
            environment->addEdge( el->first, el->second );
        }
    }
}

namespace mesh2 {

    // yuck - work around for the macro parser :(
    typedef map< Point*, Vector<3> > PointVectorMap;
    MK_GUI_COMMAND(mesh, smoothDeform, pthread_t deformThread; PointVectorMap points; static void* deformer( void* ptr ); void doSmooth( Point* pos );)
    void mesh::smoothDeform( string params ) {
        init = !init;
        if ( init ) {
            for( std::list<Point*>::iterator curr = environment->getPoints().begin();
                    curr != environment->getPoints().end(); curr++ ) {
                points[*curr] = (*curr)->getPosition();
            }
            pthread_create( &deformThread, NULL, mesh::deformer, (void*)this );
        } else if ( environment->getPoints().size() > 0 ) {
            pthread_join( deformThread, NULL );
        }
    }

    void* mesh::deformer( void* ptr ) {
        mesh* p = static_cast<mesh*>( ptr );
        Vector<3> projection;
        SE3<> camera( p->environment->getCameraPose() );
        Point* target = p->environment->sortPoints( camera ).front();
        // start point on list ~ camera + view*t
        Matrix<> rot = camera.get_rotation().get_matrix();
        projection = target->getPosition();
        projection -= camera.get_translation();
        projection[0] /= rot[0][2];
        projection[1] /= rot[1][2];
        projection[2] /= rot[2][2];
        while( p->init ) {
            camera = p->environment->getCameraPose();
            rot = camera.get_rotation().get_matrix();
            // Now project as camera + view * startPt
            // Calculate in a separate list to prevent flickering
            Vector<3> tmp( camera.get_translation() );
            tmp[0] += rot[0][2] * projection[0];
            tmp[1] += rot[1][2] * projection[1];
            tmp[2] += rot[2][2] * projection[2];
            target->setPosition( tmp );

            // Now smooth the resultant mesh
            p->doSmooth( target );
        }
        return NULL;
    }

    void mesh::doSmooth( Point* target ) {
        double attraction = GV3::get<double>( "smoothDeformAttraction", 0.8 );
        double inertia = GV3::get<double>( "smoothDeformInertia", 0.5 );
        Vector<3> pos = target->getPosition();
        Vector<3> targetMotion = pos - points[target];
        double targetWt = min( 1.0, targetMotion * targetMotion / inertia );
        // First, find the max/min weights so we can normalise to (0,1)
        double maxWt = numeric_limits<double>::min();
        double minWt = numeric_limits<double>::max();
        for( std::list<Point*>::iterator curr = environment->getPoints().begin();
                curr != environment->getPoints().end(); curr++ ) {
            Vector<3> diff( pos - points[*curr] );
            double d = diff * diff / attraction;
            maxWt = max( maxWt, d );
            minWt = min( minWt, d );
        }
        environment->lock();
        for( std::list<Point*>::iterator curr = environment->getPoints().begin();
                curr != environment->getPoints().end(); curr++ ) {
            if ( (*curr) == target )
                continue; // skip this one, must remain *on* the cursor
            Vector<3> diff( pos - points[*curr] );
            double wt = (diff * diff / attraction - minWt) / (maxWt-minWt);
            wt = targetWt * (1.0 - min( max( 0.0, wt ), 1.0 ));
            if ( wt == 0.0 )
                continue;
            (*curr)->setPosition( points[*curr] + diff * wt );
        }
        environment->unlock();
    }
}

namespace mesh3 {
    MK_GUI_COMMAND(mesh, move, SE3<> start; pthread_t mover; static void* moveProcessor( void* ptr );)
    void mesh::move( string params ) {
        if ( environment->getPoints().size() == 0 )
            return;
        if ( !init ) {
            start = environment->getCameraPose();
            init = true;
            pthread_create( &mover, NULL, mesh::moveProcessor, (void*)this );
        } else {
            init = false;
            pthread_join( mover, NULL );
        }
    }

    void* mesh::moveProcessor( void* ptr ) {
        mesh *p = static_cast<mesh*>( ptr );
        Vector<3> projection;
        SE3<> camera( p->environment->getCameraPose() );
        Point* target = p->environment->sortPoints( camera ).front();
        Vector<3> targetStart = target->getPosition();
        // start point on list ~ camera + view*t
        Matrix<> rot = camera.get_rotation().get_matrix();
        projection = target->getPosition();
        projection -= camera.get_translation();
        projection[0] /= rot[0][2];
        projection[1] /= rot[1][2];
        projection[2] /= rot[2][2];

        map< Point*, Vector<3> > targetPoints;
         for( std::list<Point*>::iterator curr = p->environment->getPoints().begin();
                    curr != p->environment->getPoints().end(); curr++ ) {
            targetPoints[*curr] = (*curr)->getPosition();
        }
        
        while( p->init ) {
            camera = p->environment->getCameraPose();
            rot = camera.get_rotation().get_matrix();
            // Work out position of target point, as in vertex.move
            Vector<3> tmp;
            tmp[0] = (camera.get_translation()[0] + rot[0][2] * projection[0]);
            tmp[1] = (camera.get_translation()[1] + rot[1][2] * projection[1]);
            tmp[2] = (camera.get_translation()[2] + rot[2][2] * projection[2]);
            // Now apply the same vector to all points
            tmp -= targetStart;
            p->environment->lock();
            for( map< Point*, Vector<3> >::iterator curr = targetPoints.begin();
                    curr != targetPoints.end(); curr++ ) {
                curr->first->setPosition( curr->second + tmp );
            }
            p->environment->unlock();
        }
        return NULL;
    }
}

namespace mesh4 {
    MK_GUI_COMMAND(mesh, scale, SE3<> start; pthread_t scaler; static void* scaleProcessor( void* ptr );)
    void mesh::scale( string params ) {
        if ( environment->getPoints().size() == 0 )
            return;
        if ( params.length() > 0 ) {
            istringstream str( params );
            double factor = 1.0;
            str >> factor;
            cerr << "Scaling mesh by a factor of " << factor << endl;
            for( std::list<Point*>::iterator curr = environment->getPoints().begin();
                    curr != environment->getPoints().end(); curr++ ) {
                (*curr)->setPosition( (*curr)->getPosition() * factor );
            }
            return;
        }
        if ( !init ) {
            start = environment->getCameraPose();
            init = true;
            pthread_create( &scaler, NULL, mesh::scaleProcessor, (void*)this );
        } else {
            init = false;
            pthread_join( scaler, NULL );
        }
    }

    void* mesh::scaleProcessor( void* ptr ) {
        mesh *p = static_cast<mesh*>( ptr );
        Vector<3> projection;
        SE3<> camera( p->environment->getCameraPose() );
        Point* target = p->environment->sortPoints( camera ).front();
        Vector<3> targetStart = target->getPosition();
        // start point on list ~ camera + view*t
        Matrix<> rot = camera.get_rotation().get_matrix();
        projection = target->getPosition();
        projection -= camera.get_translation();
        projection[0] /= rot[0][2];
        projection[1] /= rot[1][2];
        projection[2] /= rot[2][2];

        map< Point*, Vector<3> > targetPoints;
         for( std::list<Point*>::iterator curr = p->environment->getPoints().begin();
                    curr != p->environment->getPoints().end(); curr++ ) {
            targetPoints[*curr] = (*curr)->getPosition();
        }
        
        while( p->init ) {
            camera = p->environment->getCameraPose();
            rot = camera.get_rotation().get_matrix();
            // Work out position of target point, as in vertex.move
            Vector<3> tmp;
            tmp[0] = (camera.get_translation()[0] + rot[0][2] * projection[0]);
            tmp[1] = (camera.get_translation()[1] + rot[1][2] * projection[1]);
            tmp[2] = (camera.get_translation()[2] + rot[2][2] * projection[2]);

            // Now calculate the scale vector to apply to all points
            double scaleFactor = (tmp - targetStart) * (tmp - targetStart);

            p->environment->lock();
            for( map< Point*, Vector<3> >::iterator curr = targetPoints.begin();
                    curr != targetPoints.end(); curr++ ) {
                curr->first->setPosition( curr->second * scaleFactor );
            }
            p->environment->unlock();
        }
        return NULL;
    }
}

namespace code1 {
    MK_GUI_COMMAND(code, save,)
    void code::save( string filename ) {
        if ( filename.length() < 1 )
            filename = "scanner_model.out";
        ofstream file;
        map<Point*, int> points;
        file.open( filename.c_str(), ios::out | ios::trunc );
        int i = 0;
        for( std::list<Point*>::iterator curr = environment->getPoints().begin();
                curr != environment->getPoints().end(); curr++, i++ ) {
            points[*curr] = i;
            file << "vertex.create " << (*curr)->getPosition() << endl;
        }
        for( std::list<Edge*>::iterator curr = environment->getEdges().begin();
                curr != environment->getEdges().end(); curr++, i++ ) {
            file << "edge.connect " << points[(*curr)->getStart()] << " " << points[(*curr)->getEnd()] << endl;
        }
        file.close();
        cerr << "File written to " << filename << endl;
    }
}

namespace code2 {
    MK_GUI_COMMAND(code, load,)
    void code::load( string filename ) {
        if ( filename.length() < 1 )
            filename = "scanner_model.out";
        GUI.LoadFile( filename );
        cerr << "Loaded from " << filename << endl;
    }
}

namespace obj1 {
    /*
     * Note here that we save three files, with the same basename; an OBJ file
     *   for the geometry, an MTL file to define our texture mapping and how
     *   we want it to look, and a TGA image of the actual texture data.
     *
     * As texture is stored as up to getFaces().size() video frames in memory,
     *   they will be spliced together one after another in the TGA image,
     *   vertically. The texture co-ordinates in the OBJ file must assume
     *   this to be the case and normalise accordingly.
     */
    MK_GUI_COMMAND(obj, save, void saveOBJ( string filename, map< Image< Rgb<byte> > *,int> textures ); \
        void saveMTL( string filename ); void saveTGA( string filename, map< Image< Rgb<byte> > *,int>& textures );)
    void obj::save( string filename ) {
        if ( filename.length() < 1 )
            filename = "scanner_model";

        // Clean our textures first, to minimise its size
        GUI.ParseLine( "texture.clean" );

        // Furst, serialise textures as concat TGA, storing mapping of offsets
        map< Image< Rgb<byte> >*,int> textures;
        saveTGA( filename, textures );
        // Write the geometry to an OBJ file first
        saveOBJ( filename, textures );
        // Now the material database
        saveMTL( filename );

        cerr << "File written to " << filename << endl;
    }

    void obj::saveOBJ( string filename, map< Image< Rgb<byte> >*,int> textures ) {
        ofstream obj;
        obj.open( (filename + ".obj").c_str(), ios::out | ios::trunc );
        obj << "#" << endl
            << "# File describes " << environment->getPoints().size()
                    << " vertices and " << environment->getFaces().size()
                    << " faces." << endl
            << "#" << endl
            << endl
            << "# Material library links:" << endl
            << "mtllib " << filename << ".mtl" << endl
            << endl
            << "# Vertices:" << endl;
        map<Point*, int> vtxIndex;
        int i = 1; // OBJ uses 1-based indexing
        for( std::list<Point*>::iterator curr = environment->getPoints().begin();
                curr != environment->getPoints().end(); curr++, i++ ) {
            vtxIndex[*curr] = i;
            obj << "v " << (*curr)->getPosition() << endl;
        }

        obj << endl
            << "# Texture Co-Ordinates:" << endl;
        int nf = textures.size();
        // Co-ords are scaled by nf in the y direction (stacked frames), and
        // mirrored on each frame to convert from LH to RH co-ord system
        for( std::set<PolyFace*>::iterator curr = environment->getFaces().begin();
                curr != environment->getFaces().end(); curr++ ) {
            int i = 1 + textures[(*curr)->getTextureSource()];
            Vector<2> coord = (*curr)->getP1Coord( environment->getCamera() );
            coord[1] = (-coord[1] + i) / nf;
            obj << "vt " << coord << endl;
            coord = (*curr)->getP2Coord( environment->getCamera() );
            coord[1] = (-coord[1] + i) / nf;
            obj << "vt " << coord << endl;
            coord = (*curr)->getP3Coord( environment->getCamera() );
            coord[1] = (-coord[1] + i) / nf;
            obj << "vt " << coord << endl;
        }
                
        obj << endl
            << "# Normals (currently one per face):" << endl;
        for( std::set<PolyFace*>::iterator curr = environment->getFaces().begin();
                curr != environment->getFaces().end(); curr++ ) {
            obj << "vn " << (*curr)->getFaceNormal() << endl;
        }
        obj << endl
            << "# Faces:" << endl
            << "g " << filename << endl
            << "usemtl TextureMapped" << endl;
        i = 0; // Face index, to calculate which texture co-ord to link to
        for( std::set<PolyFace*>::iterator curr = environment->getFaces().begin();
                curr != environment->getFaces().end(); curr++, i++ ) {
            if ( (*curr)->hasFlippedNormal() )
                obj << "f " << vtxIndex[(*curr)->getP1()] << "/" << 3*i+1 << "/" << i+1 << " "
                        << vtxIndex[(*curr)->getP3()] << "/" << 3*i+3 << "/" << i+1 << " "
                        << vtxIndex[(*curr)->getP2()] << "/" << 3*i+2 << "/" << i+1 << endl;
            else
                obj << "f " << vtxIndex[(*curr)->getP1()] << "/" << 3*i+1 << "/" << i+1 << " "
                        << vtxIndex[(*curr)->getP2()] << "/" << 3*i+2 << "/" << i+1 << " "
                        << vtxIndex[(*curr)->getP3()] << "/" << 3*i+3 << "/" << i+1 << endl;
        }
        obj.close();
    };

    void obj::saveMTL( string filename ) {
        ofstream mtl;
        mtl.open( (filename + ".mtl").c_str(), ios::out | ios::trunc );
        mtl << "# Material database for " << filename << ".obj" << endl
            << "newmtl TextureMapped" << endl
            << "  Ka 1.000 1.000 1.000" << endl // Ambient
            << "  Kd 1.000 1.000 1.000" << endl // Diffuse
            << "  Ks 0.000 0.000 0.000" << endl // Specular
            << endl
            << "  d 1.0" << endl << "  Tr 1.0" << endl // Alpha transparency
            << "  illum 2" << endl // Illumination model
            << "  map_Ka " << filename << ".tga" << endl
            << "  map_Kd " << filename << ".tga" << endl;
        mtl.close();
    };

    typedef struct {
        byte idSize;           // size of ID field that follows header
        byte hasColourMap;     // type of colour map 0=none, 1=has palette
        byte imageType;        // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

        byte colourMapStart;   // first colour map entry in palette
        short int colourMapSize;    // number of colours in palette
        byte colourMapBits;    // number of bits per palette entry 15,16,24,32

        short int xOrigin;      // image x origin
        short int yOrigin;      // image y origin
        short int width;        // image width in pixels
        short int height;       // image height in pixels
        byte bpp;              // image bits per pixel 8,16,24,32
        byte descriptor;       // image descriptor bits (vh flip bits)
    } TGAHeader;

    void obj::saveTGA( string filename, map< Image< Rgb<byte> > *,int>& textures ) {
        TGAHeader head;
        head.idSize = 0;
        head.hasColourMap = 0;
        head.imageType = 2;

        head.colourMapStart = 0;
        head.colourMapSize = 0;
        head.colourMapBits = 24;

        head.xOrigin = 0;
        head.yOrigin = 0;
        // We'd like to minimise the number of frames we save here
        //   as there may be some duplicates... So first count the images to save
        int numImages = 0;
        for( std::set<PolyFace*>::reverse_iterator curr = environment->getFaces().rbegin();
                curr != environment->getFaces().rend(); curr++ ) {
            if ( textures.count( (*curr)->getTextureSource() ) == 0 )
                numImages++;
            textures[(*curr)->getTextureSource()] = 1;
        }
        textures.clear();
        head.width = environment->getSceneSize()[0];
        head.height = environment->getSceneSize()[1] * numImages;
        head.bpp = 24; // This is now a 24-bit RGB bitmap
        head.descriptor = 0;

        FILE *tga = fopen( (filename+".tga").c_str(), "wb" );

        if ( tga == NULL ) {
            cerr << "ERROR: Couldn't open TGA file for writing (" << filename << ".tga)" << endl;
            return;
        }

        // First, write the header we've built
        fwrite( &head, sizeof( TGAHeader ), 1, tga );

        int idx = 0;
        // Now, write images for each of the poly faces
        for( std::set<PolyFace*>::reverse_iterator curr = environment->getFaces().rbegin();
                curr != environment->getFaces().rend(); curr++ ) {
            if ( textures.count( (*curr)->getTextureSource() ) > 0 )
                continue;
            textures[(*curr)->getTextureSource()] = idx++;
            Image< Rgb<byte> > tex = (*curr)->getTexture();
            for( Image< Rgb<byte> >::iterator px = tex.end()-1;
                    px >= tex.begin(); px -= tex.size().x ) {
                for( int i = tex.size().x - 1; i >= 0 ; i-- ) {
                    fwrite( &((px-i)->blue), sizeof( byte ), 1, tga );
                    fwrite( &((px-i)->green), sizeof( byte ), 1, tga );
                    fwrite( &((px-i)->red), sizeof( byte ), 1, tga );
                }
            }
        }

        fclose( tga );
    };
};


namespace obj2 {
    /**
     * Custom subclass of PolyFace that won't accept new texture
     * Also has a fixed normal vector
     */
    class FixedPolyFace : public PolyFace {
    public:
        Vector<3> normal;
        Vector<2> t1, t2, t3;


        // Preserve point ordering for these faces, since they provide the
        //  correct implicit normal
        FixedPolyFace( Point* a, Point* b, Point* c ) : PolyFace( a, b, c ) {
            p1 = a;
            p2 = b;
            p3 = c;
        };

        virtual ~FixedPolyFace() {};
        
        virtual Vector<2> getP1Coord( ATANCamera* cam ) {
            return t1;
        };

        virtual Vector<2> getP2Coord( ATANCamera* cam ) {
            return t2;
        };

        virtual Vector<2> getP3Coord( ATANCamera* cam ) {
            return t3;
        };

        virtual void setTexture( Image< Rgb< byte > >& t, SE3<> vp ) {
            // no-op
        }

        void fixTexture( Image< Rgb<byte> >* t ) {
            textureSource = t;
            texture = *new Image< Rgb<byte> >( *t );
        }

        virtual Vector<3> getFaceNormal() {
            return normal;
        };
    };

    /*
     * Note here that we have to load three files, with the same basename; an
     *   OBJ file for the geometry, an MTL file to define our texture mapping
     *   and how we want it to look, and an image of the actual texture data.
     */
    MK_GUI_COMMAND(obj, load, void loadOBJ( string filename, string& mtlLib, string& mtlName, Image< Rgb<byte> > &texture ); \
        void loadMTL( string filename, string mtlName, Image< Rgb<byte> > &texture ); \
        void loadTGA( string texImg, Image< Rgb<byte> > &texture ); )
    void obj::load( string filename ) {
        if ( filename.length() < 1 )
            filename = "scanner_model.obj";

        string mtlLib = "UNKNOWN";
        string mtlName = "DEFAULT";
        string texImg = "UNKNOWN";
        Image< Rgb<byte> > texture = *new Image< Rgb<byte> >(
                ImageRef( 640, 480 ), Rgb<byte>( 0, 255, 0 ) );

        environment->lock();
        loadOBJ( filename, mtlLib, mtlName, texture );
        cerr << "Geometry loaded. Loading MTL." << endl;
        loadMTL( mtlLib, mtlName, texture, texImg );
        loadTGA( texImg, texture );
        environment->unlock();

        cerr << "OBJ loaded from " << filename << endl;
    }

    void obj::loadOBJ( string filename, string& mtlLib, string& mtlName, Image< Rgb<byte> > &texture ) {
        ifstream obj;
        obj.open( filename.c_str(), ios::in );

        // Nota Bene!! IDs are 1-based, not 0-based like vector indices!
        vector<Point*> pts;           // Point IDs
        vector< Vector<3> > norm;      // Normals mapped by ID also
        vector< Vector<2> > tex;       // Texture co-ordinate IDs
        char c;

        while( obj.good() && !obj.eof() ) {
            obj >> skipws >> c;
            if ( c == '#' ) {               // Comment : Skip line
                while( c != '\n' )
                    obj >> noskipws >> c;
            } else if ( c == 'v' ) {    // vertex or normal or tex co-ord
                c = obj.peek();
                if ( c == 'n' ) {       // Normal vector
                    obj.get();
                    Vector<3> n;
                    obj >> n;
                    norm.push_back( n );
                } else if ( c == 't' ) { // Texture co-ordinate
                    obj.get();
                    Vector<2> t;
                    obj >> t;
                    tex.push_back( t );
                } else {                // Geometry vertex
                    Vector<3> pos;
                    obj >> pos;
                    Point* p = new Point( pos );
                    environment->addPoint( p );
                    pts.push_back( p );
                }
            } else if ( c == 'f' ) {    // face definition
                string p1, p2, p3; // Grab to the end of the line
                obj >> p1 >> p2 >> p3;
                istringstream fl1( p1 ), fl2( p2 ), fl3( p3 );
                int v1 = -1, v2 = -1, v3 = -1;
                int vt1 = -1, vt2 = -1, vt3 = -1;
                int vn1 = -1, vn2 = -1, vn3 = -1;
                // Get first vertex definition
                fl1 >> skipws >> v1;
                if ( fl1.peek() == '/' ) {
                    fl1.get();
                    if ( fl1.peek() == '/' ) {
                        fl1.get();
                        fl1 >> skipws >> vn1;
                    } else {
                        fl1 >> skipws >> vt1;
                        if ( fl1.peek() == '/' ) {
                            fl1.get();
                            fl1 >> skipws >> vn1;
                        }
                    }
                }
                // Second vertex definition
                fl2 >> skipws >> v2;
                if ( fl2.peek() == '/' ) {
                    fl2.get();
                    if ( fl2.peek() == '/' ) {
                        fl2.get();
                        fl2 >> skipws >> vn2;
                    } else {
                        fl2 >> skipws >> vt2;
                        if ( fl2.peek() == '/' ) {
                            fl2.get();
                            fl2 >> skipws >> vn2;
                        }
                    }
                }
                // Third vertex definition
                fl3 >> skipws >> v3;
                if ( fl3.peek() == '/' ) {
                    fl3.get();
                    if ( fl3.peek() == '/' ) {
                        fl3.get();
                        fl3 >> skipws >> vn3;
                    } else {
                        fl3 >> skipws >> vt3;
                        if ( fl3.peek() == '/' ) {
                            fl3.get();
                            fl3 >> skipws >> vn3;
                        }
                    }
                }
                bool hasNormal = vn1 > 0 && vn2 > 0 && vn3 > 0 && vn1 <= norm.size() && vn2 <= norm.size() && vn3 <= norm.size();
                bool hasTexture = vt1 > 0 && vt2 > 0 && vt3 > 0 && vt1 <= tex.size() && vt2 <= tex.size() && vt3 <= tex.size();
                if ( v1 < 0 || v2 < 0 || v3 < 0 || v1 > pts.size() || v2 > pts.size() || v3 > pts.size() ) {
                    cerr << ">> Invalid face definition! Skip." << endl;
                    cerr << "\tFace was: (" << v1 << "," << v2 << "," << v3 << ") / " << pts.size() << ". Normals: ("
                        << vn1 << "," << vn2 << "," << vn3 << ") / " << norm.size() << ". Textures: ("
                        << vt1 << "," << vt2 << "," << vt3 << ") / " << tex.size() << "." << endl;
                } else {
                    // Build PolyFace and add to the environment
                    FixedPolyFace* face = new FixedPolyFace( pts[v1-1], pts[v2-1], pts[v3-1] );
                    face->fixTexture( &texture );
                    if ( hasTexture ) {
                        face->t1 = tex[vt1-1];
                        face->t2 = tex[vt2-1];
                        face->t3 = tex[vt3-1];
                    }
                    if ( hasNormal ) {
                        face->normal = norm[vn1-1] + norm[vn2-1] + norm[vn3-1];
                    } else {
                        face->normal = (pts[v2-1]->getPosition() - pts[v1-1]->getPosition())
                                ^ (pts[v3-1]->getPosition() - pts[v1-1]->getPosition());
                    }
                    face->normal = face->normal / (face->normal * face->normal);

                    bool mkEdge = true;
                    // Edge v1 <-> v2
                    for( std::list<Edge*>::iterator curr = environment->getEdges().begin();
                            curr != environment->getEdges().end(); curr++ ) {
                        if( ((*curr)->getStart() == pts[v1-1] && (*curr)->getEnd() == pts[v2-1])
                                || ((*curr)->getStart() == pts[v2-1] && (*curr)->getEnd() == pts[v1-1]) ) {
                            mkEdge = false;
                            break;
                        }
                    }
                    if ( mkEdge ) {
                        Edge* e = new Edge( pts[v1-1], pts[v2-1] );
                        environment->getEdges().push_back( e );
                        pts[v1-1]->addEdge( e );
                        pts[v2-1]->addEdge( e );
                    }
                    mkEdge = true;
                    // Edge v2 <-> v3
                    for( std::list<Edge*>::iterator curr = environment->getEdges().begin();
                            curr != environment->getEdges().end(); curr++ ) {
                        if( ((*curr)->getStart() == pts[v3-1] && (*curr)->getEnd() == pts[v2-1])
                                || ((*curr)->getStart() == pts[v2-1] && (*curr)->getEnd() == pts[v3-1]) ) {
                            mkEdge = false;
                            break;
                        }
                    }
                    if ( mkEdge ) {
                        Edge* e = new Edge( pts[v3-1], pts[v2-1] );
                        environment->getEdges().push_back( e );
                        pts[v3-1]->addEdge( e );
                        pts[v2-1]->addEdge( e );
                    }
                    mkEdge = true;
                    // Edge v3 <-> v1
                    for( std::list<Edge*>::iterator curr = environment->getEdges().begin();
                            curr != environment->getEdges().end(); curr++ ) {
                        if( ((*curr)->getStart() == pts[v3-1] && (*curr)->getEnd() == pts[v1-1])
                                || ((*curr)->getStart() == pts[v1-1] && (*curr)->getEnd() == pts[v3-1]) ) {
                            mkEdge = false;
                            break;
                        }
                    }
                    if ( mkEdge ) {
                        Edge* e = new Edge( pts[v1-1], pts[v3-1] );
                        environment->getEdges().push_back( e );
                        pts[v1-1]->addEdge( e );
                        pts[v3-1]->addEdge( e );
                    }
                    environment->getFaces().insert( face );
                }
            } else if ( c == 'm' ) {    // mtllib
                while( obj.peek() != ' ' )
                    obj.get();
                mtlLib.clear();
                obj >> skipws >> mtlLib;
                cerr << ">> Using mtlLib = " << mtlLib << endl;
            } else if ( c == 'u' ) {    // usemtl
                while( c != ' ' )
                    obj >> noskipws >> c;
                mtlName.clear();
                obj >> skipws >> mtlName;
                cerr << ">> Using mtl = " << mtlName << endl;
            } else {                    // Skip line
                while( c != '\n' )
                    obj >> noskipws >> c;
            }
        }
        obj.close();
    };

    void obj::loadMTL( string filename, string mtlName, Image< Rgb<byte> > &texture, string texImg ) {
        ifstream mtl;
        mtl.open( filename.c_str(), ios::in );
        char c;

        while( mtl.good() && !mtl.eof() ) {
            mtl >> skipws >> c;
            if ( c == '#' ) {               // Comment : Skip line
                while( c != '\n' )
                    mtl >> noskipws >> c;
            } else if ( c == 'n' ) {        // newmtl declaration
                string test;
                while( c != ' ' )
                    mtl >> noskipws >> c;
                mtl >> skipws >> test;
                if ( test == mtlName ) {    // Found target material
                    while( mtl.good() && !mtl.eof() ) {
                        mtl >> skipws >> c;
                        if ( c == 'K' ) {
                            mtl >> c;
                            if ( c == 'a' ) {
                                // Fill the texture with the ambient colour
                                Rgb<byte> colour;
                                double r, g, b;
                                mtl >> r >> g >> b;
                                colour.red = r * 255;
                                colour.green = g * 255;
                                colour.blue = b * 255;
                                cerr << ">> Filling texture with ambient colour " << colour << endl;
                                texture.fill( colour );
                            }
                        } else if ( c == 'm' ) {
                            string texMap;
                            mtl >> texMap;
                            // (NB: "m" already consumed)
                            if ( texMap == "ap_Ka" ) { // Ambient map
                                mtl >> skipws >> texImg;
                                cerr << ">> Loading texture from " << texMap << endl;
                            }
                        } else {                    // Skip line
                            while( c != '\n' )
                                mtl >> noskipws >> c;
                        }
                    }
                    break;
                }
            } else {                    // Skip line
                while( c != '\n' )
                    mtl >> noskipws >> c;
            }
        }
        mtl.close();
    };

    void obj::loadTGA( string texImg, Image< Rgb<byte> > &texture ) {
        obj1::TGAHeader head;
        FILE *tga = fopen( filename.c_str(), "r" );
    };
};

namespace randmodel1 {
    MK_GUI_COMMAND(model, rand,)
    void model::rand( string numPts ) {
        if ( numPts.length() < 1 )
            numPts = "10";
        stringstream s( numPts );
        int pts;
        s >> pts;
        cerr << "Adding " << pts << " connected points" << endl;

        double edgeProb = GV3::get<double>( "edgeProb", 0.5 );
        for (int i = 0; i < pts; i++) {
            Point* p = new Point(
                    2 * static_cast<double>( std::rand() ) / RAND_MAX - 0.5,
                    2 * static_cast<double>( std::rand() ) / RAND_MAX - 0.5,
                    2 * static_cast<double>( std::rand() ) / RAND_MAX - 0.5 );
            environment->addPoint( p );
            for( std::list<Point*>::iterator inner = environment->getPoints().begin();
                    inner != environment->getPoints().end(); inner++ ) {
                if ( static_cast<double>( std::rand() ) / RAND_MAX < edgeProb )
                    environment->addEdge( p, *inner );
            }
        }
        cerr << "Model size: " << environment->getPoints().size() << " points, "
                << environment->getEdges().size() << " edges, and "
                << environment->getFaces().size() << " faces." << endl;
    }
};

namespace randmodel2 {
    MK_GUI_COMMAND(model, randFaces,)
    void model::randFaces( string numFaces ) {
        if ( numFaces.length() < 1 )
            numFaces = "10";
        stringstream s( numFaces );
        int faces;
        s >> faces;
        cerr << "Adding " << faces << " new faces" << endl;
        Point* p3 = new Point(
                    2 * static_cast<double>( std::rand() ) / RAND_MAX - 0.5,
                    2 * static_cast<double>( std::rand() ) / RAND_MAX - 0.5,
                    2 * static_cast<double>( std::rand() ) / RAND_MAX - 0.5 );
        environment->addPoint( p3 );
        Point* p2 = new Point(
                    2 * static_cast<double>( std::rand() ) / RAND_MAX - 0.5,
                    2 * static_cast<double>( std::rand() ) / RAND_MAX - 0.5,
                    2 * static_cast<double>( std::rand() ) / RAND_MAX - 0.5 );
        environment->addPoint( p2 );
        environment->addEdge( p3, p2 );
        for (int i = 0; i < faces; i++) {
            Point* p = new Point(
                    2 * static_cast<double>( std::rand() ) / RAND_MAX - 0.5,
                    2 * static_cast<double>( std::rand() ) / RAND_MAX - 0.5,
                    2 * static_cast<double>( std::rand() ) / RAND_MAX - 0.5 );
            environment->addPoint( p );
            environment->addEdge( p, p2 );
            environment->addEdge( p, p3 );
            p3 = p2;
            p2 = p;
        }
        cerr << "Model size: " << environment->getPoints().size() << " points, "
                << environment->getEdges().size() << " edges, and "
                << environment->getFaces().size() << " faces." << endl;
    }
};

MK_GUI_COMMAND( errormap, save, int sessionID; int currFrame; )
void errormap::save( string params ) {
    if ( !init ) {
        init = true;
        sessionID = rand();
        currFrame = 0;
    }
    stringstream fname;
    fname << "errormap_" << sessionID << "_" << currFrame++ << ".png";
    cerr << "Saving error map to " << fname.str() << endl;

    img_save( accuracy::instance->rendered, fname.str() );
}
