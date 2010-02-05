
#include "AIUnit.h"
#include <gvars3/gvars3.h>
using namespace GVars3;

AIUnit::AIUnit( WorldMap* m ) : velocity( 0 ), xPos( 0 ), yPos( 0 ), zPos( 0 ), map( m ), search( m ) {
}

AIUnit::AIUnit( WorldMap* m, double x, double y, double z ) : velocity( 0 ), xPos( x ), yPos( y ), zPos( z ), map( m ), search( m ) {
}

AIUnit::~AIUnit() {
}

#define ABS( a ) (a > 0 ? a : -a)
#define ABSDIFF( a, b ) (a > b ? a - b : b - a)
void AIUnit::tick() {
    if ( path.size() < 1 )
        return;

    velocity = GV3::get<double>( "aiSpeed", 0.01 );
    cerr << "tock @ " << xPos << ", " << yPos << ", " << zPos << endl;
    if ( ABSDIFF( path.front()->x, xPos ) < 0.01
            && ABSDIFF( path.front()->y, yPos ) < 0.01
            && ABSDIFF( path.front()->z, zPos ) < 0.01 ) {
        cerr << "\tPath Point Reached: ";
        // Test if this is a goal object to be deleted (i.e. not in the node graph)
        if ( path.front()->traversable.size() == 0 )
            delete path.front();
        path.pop_front();
        // Check if we've reached our goal
        if ( path.size() == 0 ) {
            cerr << "STOP" << endl;
            velocity = 0;
            return;
        }
        cerr << "Next stop = " << path.front()->x << ", " << path.front()->y << ", " << path.front()->z << " -> " << endl;
    }
    // Set up new {x,y,z}Dir
    xDir = path.front()->x - xPos;
    yDir = path.front()->y - yPos;
    zDir = path.front()->z - zPos;
    // Normalise the vector
    double div = ABS( xDir ) + ABS( yDir ) + ABS( zDir );
    xDir /= div;
    yDir /= div;
    zDir /= div;
    cerr << "\tDIR = " << xDir << ", " << yDir << ", " << zDir << endl;
    xPos += velocity * xDir;
    yPos += velocity * yDir;
    zPos += velocity * zDir;
};

double AIUnit::getX() {
    return xPos;
};

double AIUnit::getY() {
    return yPos;
};

double AIUnit::getZ() {
    return zPos;
};

void AIUnit::navigateTo( Waypoint* goal ) {
    Waypoint* from = new Waypoint;
    from->x = xPos;
    from->y = yPos;
    from->z = zPos;
    path = search.findPath( from, goal );
    path.push_front( from );
    if( path.back()->x != goal->x || path.back()->y != goal->y || path.back()->z != goal->z )
        path.push_back( goal );
    cerr << "Got path of " << path.size() << " points" << endl;
};