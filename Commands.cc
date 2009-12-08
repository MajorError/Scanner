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

MK_GUI_COMMAND(target, create,)
void target::create( string params ) {
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
        std::vector< Vector<3> > features( environment->getFeaturesSorted( environment->getCameraPose(), rad ) );
        if ( features.size() < 1 )
            return;
        v = features[0];
        cout << "target.create " << v[0] << ' ' << v[1] << ' ' << v[2] << endl;
    }
    environment->addPoint( new Point( v ) );
}

MK_GUI_COMMAND(point, move, SE3<> start; bool working; pthread_t mover; static void* moveProcessor( void* ptr );)
void point::move( string params ) {
    if ( !working ) {
        start = environment->getCameraPose();
        working = true;
        pthread_create( &mover, NULL, point::moveProcessor, (void*)this );
    } else if ( environment->getPoints().size() > 0 ) {
        working = false;
        pthread_join( mover, NULL );
    }
}

void* point::moveProcessor( void* ptr ) {
    point *p = static_cast<point*>( ptr );
    Vector<3> projection;
    SE3<> camera( p->environment->getCameraPose() );
    Point* target = p->environment->sortPoints( camera )[0];
    // start point on vector ~ camera + view*t
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
        // Calculate in a separate vector to prevent flickering
        Vector<3> tmp( camera.get_translation() );
        tmp[0] += rot[0][2] * projection[0];
        tmp[1] += rot[1][2] * projection[1];
        tmp[2] += rot[2][2] * projection[2];
        target->setPosition( tmp );
    }
    return NULL;
}

MK_GUI_COMMAND(edge, connect, Point* from; bool complete; )
void edge::connect( string params ) {
    if ( environment->getPoints().size() < 2 )
         return;
    if ( !complete ) {
        from = environment->sortPoints( environment->getCameraPose() )[0];
        complete = true;
    } else {
        Point* to = environment->sortPoints( environment->getCameraPose() )[0];
        environment->addEdge( from, to );
        complete = false;
    }
}

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
