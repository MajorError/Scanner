#include <GL/gl.h>
#include <fstream>

#include "Point.h"
#include "Environment.h"
#include "GUICommand.h"
#include "ARDriver.h"
#include "Game/GameRenderer.h"
#include "Game/WorldMap.h"
#include "Game/GameFactory.h"

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
    MK_GUI_COMMAND(vertex, move, SE3<> start; pthread_t mover; static void* moveProcessor( void* ptr );)
    void vertex::move( string params ) {
        if ( !init ) {
            start = environment->getCameraPose();
            init = true;
            pthread_create( &mover, NULL, vertex::moveProcessor, (void*)this );
        } else if ( environment->getPoints().size() > 0 ) {
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
    MK_GUI_COMMAND(obj, save, void saveOBJ( string filename ); \
        void saveMTL( string filename ); void saveTGA( string filename );)
    void obj::save( string filename ) {
        if ( filename.length() < 1 )
            filename = "scanner_model";
        
        // Write the geometry to an OBJ file first
        saveOBJ( filename );
        // Now the material database
        saveMTL( filename );
        // Finally, serialise textures as TGA images
        saveTGA( filename );

        cerr << "File written to " << filename << endl;
    }

    void obj::saveOBJ( string filename ) {
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
        i = 0; // Face index, to calculate texture offset in TGA file
        int nf = environment->getFaces().size();
        // Co-ords are scaled by nf in the y direction (stacked frames), and
        // mirrored in the x direction (we write data backwards)
        for( std::set<PolyFace*>::iterator curr = environment->getFaces().begin();
                curr != environment->getFaces().end(); curr++, i++ ) {
            Vector<2> coord = (*curr)->getP1Coord( environment->getCamera() ) + makeVector( 0, i );
            coord[1] /= nf;
            obj << "vt " << coord << endl;
            coord = (*curr)->getP2Coord( environment->getCamera() ) + makeVector( 0, i );
            coord[1] /= nf;
            obj << "vt " << coord << endl;
            coord = (*curr)->getP3Coord( environment->getCamera() ) + makeVector( 0, i );
            coord[1] /= nf;
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

    void obj::saveTGA( string filename ) {
        TGAHeader head;
        head.idSize = 0;
        head.hasColourMap = 0;
        head.imageType = 2;

        head.colourMapStart = 0;
        head.colourMapSize = 0;
        head.colourMapBits = 24;

        head.xOrigin = 0;
        head.yOrigin = 0;
        // Ideally, we'd like to minimise the number of frames we save here
        //   as there may be some duplicates... This can be a TODO for later!
        head.width = environment->getSceneSize()[0];
        head.height = environment->getSceneSize()[1] * environment->getFaces().size();
        head.bpp = 24; // This is now a 24-bit RGB bitmap
        head.descriptor = 0;

        FILE *tga = fopen( (filename+".tga").c_str(), "wb" );

        if ( tga == NULL ) {
            cerr << "ERROR: Couldn't open TGA file for writing (" << filename << ".tga)" << endl;
            return;
        }

        // First, write the header we've built
        fwrite( &head, sizeof( TGAHeader ), 1, tga );

        // Now, write images for each of the poly faces
        for( std::set<PolyFace*>::reverse_iterator curr = environment->getFaces().rbegin();
                curr != environment->getFaces().rend(); curr++ ) {
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
}