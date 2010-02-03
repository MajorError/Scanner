
#include "AStarSearch.h"
#include <iostream>

AStarSearch::AStarSearch( WorldMap &m ) : wm( m ) {
};

AStarSearch::~AStarSearch() {
};


list<Waypoint*> AStarSearch::findPath( Waypoint* from, Waypoint* to ) {
    set<Waypoint*> open;
    set<Waypoint*> closed;
    map<Waypoint*,AStarElement> els;
    // TODO: If from / to aren't in the map, find them first

    open.insert( from );
    els[from].parent = NULL;
    els[from].distanceFromStart = 0.0;
    els[from].distanceToTarget = (*from) - to;

    // Perform the search
    Waypoint* currPt = NULL;
    while( currPt != to ) {
        // Find current best point
        double bestDistance = numeric_limits<double>::max();
        for( set<Waypoint*>::iterator curr = open.begin(); curr != open.end(); curr++ ) {
            double d = els[*curr].distanceToTarget + els[*curr].distanceFromStart;
            if ( d < bestDistance ) {
                currPt = *curr;
                bestDistance = d;
            }
        }

        // Add the current point to the closed list
        closed.insert( currPt );
        open.erase( currPt );

        // Walk adjacent nodes to the current point
        for( list<Waypoint*>::iterator curr = currPt->traversable.begin();
                curr != currPt->traversable.end(); curr++ ) {
            // Skip node if it's already closed
            if ( closed.count( *curr ) > 0 )
                continue;
            
            double currStartDistance = els[currPt].distanceFromStart + ((*currPt) - *curr);
            if ( open.count( *curr ) < 1 ) {
                els[*curr].parent = currPt;
                els[*curr].distanceFromStart = currStartDistance;
                els[*curr].distanceToTarget = (*(*curr)) - to;
                open.insert( *curr );
            } else if ( els[*curr].distanceFromStart > currStartDistance ) {
                open.insert( *curr );
                els[*curr].parent = currPt;
                els[*curr].distanceFromStart = currStartDistance;
            }
        }

        // Check for empty open list; if found, return a path to the best
        //   point we could find (closest to target)
        if ( open.size() < 1 ) {
            cerr << "WARNING: Search failed to find path!" << endl;
            bestDistance = numeric_limits<double>::max();
            for( set<Waypoint*>::iterator curr = closed.begin(); curr != closed.end(); curr++ ) {
                if ( els[*curr].distanceToTarget < bestDistance ) {
                    to = *curr;
                    bestDistance = els[to].distanceToTarget;
                }
            }
            break;
        }
    }

    // Iterate backwards through the search to find the path
    list<Waypoint*> path;
    Waypoint* curr = to;
    path.push_back( to );
    while( curr != from && curr != NULL ) {
        curr = els[curr].parent;
        path.push_front( curr );
    }
    return path;
};
