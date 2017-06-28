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
//   T::VSize - total number of vertices
//   graph.adjacent(vertex) - return iterator of edges adjacent to vertex
//   graph.dest(edge)       - return destination of edge
//   graph.weight(edge)     - return weight of edge
template <typename T>
void dijkstra(const T &graph, unsigned char start, Path (&path_out)[T::VSize]) {

    struct WeightedVertex {
        short weight;
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
        p.parent = -1;
        p.distance = -1;
    }

    // Push the first node onto the heap.
    Heap<T::VSize, WeightedVertex, Comp> heap;
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
            /* .parent =   */ front.parentIdx,
            /* .distance = */ front.weight,
        };

        // Push all the adjacent nodes onto the heap.
        const size_t n = graph.adjacentN(front.vertexIdx);
        for (size_t i = 0; i < n; i++) {
            const auto &edge = graph.adjacent(front.vertexIdx, i);
            unsigned char vertexIdx = graph.dest(edge);
            short weight = graph.weight(edge);

            // Skip if we have already found a path to this node.
            if (path_out[vertexIdx].parent != -1) {
                continue;
            }

            heap.push(WeightedVertex{
                /* .weight    = */ static_cast<short>(front.weight + weight),
                /* .vertexIdx = */ vertexIdx,
                /* .parentIdx = */ front.vertexIdx,
            });
        }
    }
}
