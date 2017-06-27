#pragma once

#include "heap.h"
#include "std.h"

using ctl::size_t;
using ctl::Heap;

// Path represents a tree.
struct Path {
    // Index to the parent node. The start node has itself as the parent.
    // Unreachable nodes have -1 as their parent.
    short parent;

    // Distance to this node from the start node. Unreachable nodes have a
    // value of -1.
    short distance;
};

// dijkstra find shortest path from start node to all other nodes.
//
// Parameters:
//   T - traits class
//   VSize    - size of the vertices array
//   vertices - array of vertices of static size
//   start    - index of the first node
//   path     - output path, must have the same size as vertices
//
// The traits are:
//   T::V - type of the vertex
//   T::E - type of the edge
//   T::adjacent(vertex) - return iterator of edges adjacent to vertex
//   T::dest(edge)       - return destination of edge
//   T::weight(edge)     - return weight of edge
template <typename T, size_t VSize>
void dijkstra(const typename T::V (&vertices)[VSize], int start, Path (&path_out)[VSize]) {

    struct WeightedVertex {
        unsigned short weight;
        unsigned char vertexIdx;
        unsigned char parentIdx;
    };

    struct Comp {
        bool operator()(const WeightedVertex &lhs, const WeightedVertex &rhs) const {
            return lhs.weight < rhs.weight;
        }
    };

    // Reset the output array in case it has already been used.
    for (auto &p : path_out) {
        path_out.parent = -1;
        path_out.distance = -1;
    }

    // Push the first node onto the heap.
    Heap<VSize, WeightedVertex, Comp> heap;
    heap.push(WeightedVertex{
        /* .weight    = */ 0,
        /* .vertexIdx = */ start,
        /* .parentIdx = */ start,
    });

    while (!heap.empty()) {
        // Pop the shortest path from the heap.
        WeightedVertex front = heap.pop();

        // Update the weight and parent.
        path_out[front.vertexIdx] = Path{
            /* .distance = */ front.weight,
            /* .parent =   */ front.parentIdx,
        };

        // Push all the adjacent nodes onto the heap.
        for (const typename T::E &edge : T::adjacent(vertices)) {
            unsigned char vertexIdx = T::dest(edge);
            unsigned short weight = T::weight(edge);

            // Skip if we have already found a path to this node.
            if (path_out[vertexIdx] != -1) {
                continue;
            }

            heap.push(WeightedVertex{
                /* .weight    = */ front.weight + weight,
                /* .vertexIdx = */ vertexIdx,
                /* .parentIdx = */ front.vertexIdx,
            });
        }
    }
}
