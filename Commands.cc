

MK_GUI_COMMAND(key, handle,)
void key::handle( string s ) {
    for( unsigned int i = 0; i < Tool::list.size(); i++ ) {
        if ( Tool::list[i]->getHotKey() == s )
            Tool::list[i]->processClick();
    }
    // Chain all commands through to super KeyPress method (from PTAM)
    GUI.ParseLine( "KeyPress "+s );
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
    environment->addPoint( v );
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
    Matrix<> rot = camera.get_rotation().get_matrix();
    Vector<3> view = makeVector( rot[0][2], rot[1][2], rot[2][2] );
    // Set up and perform the sort
    Environment::v = camera.get_translation();
    Environment::o = camera.get_translation() + view;
    std::sort( p->environment->getPoints().begin(), p->environment->getPoints().end(), Environment::closer );
    // start point on vector ~ camera + view*t
    projection = p->environment->getPoints()[0];
    projection -= camera.get_translation();
    projection[0] /= view[0];
    projection[1] /= view[1];
    projection[2] /= view[2];
    cerr << "Move Factor: " << projection << endl;
    while( p->working ) {
        camera = p->environment->getCameraPose();
        rot = camera.get_rotation().get_matrix();
        // Now project as camera + view * startPt
        // Calculate in a separate vector to prevent flickering
        Vector<3> tmp( camera.get_translation() );
        tmp[0] += rot[0][2] * projection[0];
        tmp[1] += rot[1][2] * projection[1];
        tmp[2] += rot[2][2] * projection[2];
        p->environment->getPoints()[0] = tmp;
    }
    return NULL;
}
