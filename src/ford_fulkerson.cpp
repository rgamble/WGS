// This file is part of WGS, a customizable Word Game Solver.
//
// Copyright 2014 Robert Gamble <rgamble99@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <utility>
#include <cstring>
using namespace std;

class fordFulkerson {
    int *capacity_matrix; 
    int *flow_network;

    int *q;
    int qf;
    int qb;
    int *prev;
    int vertices;

public:
    fordFulkerson(int _vertices) {
        vertices = _vertices;
        capacity_matrix = new int [vertices * vertices];
        flow_network = new int [vertices * vertices];
        q = new int[vertices];
        prev = new int[vertices];

        memset(capacity_matrix, 0, vertices * vertices * sizeof(*capacity_matrix));
    }
        
    ~fordFulkerson() {
        delete [] capacity_matrix;
        delete [] flow_network;
        delete [] q;
        delete [] prev;
    }

    void addEdge(int u, int v) {
        capacity_matrix[u * vertices + v] = 1;
    }

    void removeEdge(int u, int v) {
        capacity_matrix[u * vertices + v] = 0;
    }

    void clear() {
        memset(capacity_matrix, 0, vertices * vertices * sizeof(*capacity_matrix));
    }

    size_t maxFlow(int source, int sink)
    {
        // initialize the flow network
        memset(flow_network, 0, vertices * vertices * sizeof(*flow_network));

        size_t flow = 0;

        while(true) {
            // find an augmenting path
            for (int i = 0; i < vertices; i++)
                prev[i] = -1;

            qf = qb = 0;
            prev[q[qb++] = source] = -2;

            while(qb > qf && prev[sink] == -1) {
                for(int u = q[qf++], v = 0; v < vertices; v++) {
                    if( prev[v] == -1
                        && flow_network[u * vertices + v] - flow_network[v * vertices + u]
                            < capacity_matrix[u * vertices + v] ) {
                        prev[q[qb++] = v] = u;
                    }
                }
            }

            // see if we're done
            if( prev[sink] == -1 ) break;

            // get the bottleneck capacity
            int bot = 0x7FFFFFFF;
            for(int v = sink, u = prev[v]; u >= 0; v = u, u = prev[v]) {
                int new_bot = capacity_matrix[u * vertices + v] - flow_network[u * vertices + v] +
                    flow_network[v * vertices + u];
                if (bot > new_bot) {
                    bot = new_bot;
                }
            }

            // update the flow network
            for(int v = sink, u = prev[v]; u >= 0; v = u, u = prev[v]) {
                flow_network[u * vertices + v] += bot;
            }

            flow += bot;
        }

        return flow;
    }
};
