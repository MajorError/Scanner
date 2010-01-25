#include <GL/gl.h>

#include "Point.h"
#include "Environment.h"
#include "GUICommand.h"

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
    MK_GUI_COMMAND(vertex, move, SE3<> start; bool working; pthread_t mover; static void* moveProcessor( void* ptr );)
    void vertex::move( string params ) {
        if ( !working ) {
            start = environment->getCameraPose();
            working = true;
            pthread_create( &mover, NULL, vertex::moveProcessor, (void*)this );
        } else if ( environment->getPoints().size() > 0 ) {
            working = false;
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
        while( p->working ) {
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
    MK_GUI_COMMAND(edge, connect, Point* from; bool start; )
    void edge::connect( string params ) {
        if ( environment->getPoints().size() < 2 )
             return;
        if ( start ) {
            cerr << "edge.connect A" << endl;
            from = environment->sortPoints( environment->getCameraPose() ).front();
            start = false;
        } else {
            cerr << "edge.connect B" << endl;
            Point* to = environment->sortPoints( environment->getCameraPose() ).front();
            environment->addEdge( from, to );
            start = true;
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

namespace {
    MK_GUI_COMMAND(text, draw,)
    void text::draw( string text ) {
        ImageRef centre( environment->getSceneSize() );
        centre *= 0.5;
        centre.y += 10;
        centre.x -= text.length() / 2;
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glTranslate(centre);
        glScalef(10,-10,1);
        glColor4d( 1.0, 1.0, 1.0, 1.0 );
        glDrawText(text, FILL, 1.8, 0.2);
        glPopMatrix();
    }
}

namespace {
    MK_GUI_COMMAND(textures, clean,)
    void textures::clean( string args ) {
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
