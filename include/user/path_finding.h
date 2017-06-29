#pragma once

#include "heap.h"
#include "std.h"

using ctl::size_t;
using ctl::Heap;

typedef unsigned char NodeIdx;

struct Path {
    short nodeIdx;
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
//
// Returns: length of the path, or 0 if no path found
template <typename T>
int dijkstra(const T &graph, NodeIdx start, NodeIdx end, Path (&path_out)[T::VSize]) {

    struct alignas(4) WeightedVertex {
        short weight;
        NodeIdx vertexIdx;
        NodeIdx parentIdx;
        unsigned char nNodes; // number of nodes in this path
    };

    struct Comp {
        bool operator()(const WeightedVertex &lhs, const WeightedVertex &rhs) const {
            return lhs.weight < rhs.weight;
        }
    };

    struct Tree {
        // Index to the parent node. The start node has itself as the parent.
        // Unreachable nodes have -1 as their parent.
        short parent = -1;

        // Distance to this node from the start node. Unreachable nodes have a
        // value of -1.
        short distance = -1;

        unsigned char nNodes;
    } tree[T::VSize];

    // Push the first node onto the heap.
    Heap<T::VSize, WeightedVertex, Comp> heap;
    heap.push(WeightedVertex{
        /* .weight    = */ 0,
        /* .vertexIdx = */ start,
        /* .parentIdx = */ start,
        /* .nNodes    = */ 1,
    });

    while (!heap.empty()) {
        // Pop the shortest path from the heap.
        WeightedVertex front = heap.pop();

        if (tree[front.vertexIdx].parent != -1) continue;

        // Update the weight and parent.
        tree[front.vertexIdx] = Tree{
            /* .parent   = */ front.parentIdx,
            /* .distance = */ front.weight,
            /* .nNodes   = */ front.nNodes,
        };

        // Exit early if we reached the end node.
        if (front.vertexIdx == end) {
            break;
        }

        // Push all the adjacent nodes onto the heap.
        const size_t n = graph.adjacentN(front.vertexIdx);
        for (size_t i = 0; i < n; i++) {
            const auto &edge = graph.adjacent(front.vertexIdx, i);
            NodeIdx vertexIdx = graph.dest(edge);
            short weight = graph.weight(edge);

            // Skip if we have already found a path to this node.
            if (tree[vertexIdx].parent != -1) {
                continue;
            }

            heap.push(WeightedVertex{
                /* .weight    = */ static_cast<short>(front.weight + weight),
                /* .vertexIdx = */ vertexIdx,
                /* .parentIdx = */ front.vertexIdx,
                /* .nNodes    = */ static_cast<unsigned char>(front.nNodes + 1),
            });
        }
    }

    if (tree[end].parent == -1) {
        return 0;
    }

    // Reverse path.
    NodeIdx curIdx = end;
    for (int i = tree[end].nNodes - 1; i >= 0; i--) {
        path_out[i].nodeIdx = curIdx;
        path_out[i].distance = tree[end].distance - tree[curIdx].distance;
        curIdx = tree[curIdx].parent;
    }

    return tree[end].nNodes;
}
