#pragma once

#include "heap.h"
#include "std.h"

using ctl::Heap;

typedef U8 NodeIdx;

struct Path {
    I16 nodeIdx;
    I16 distance;
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
template <typename T, Size PathSize>
int dijkstra(const T &graph, NodeIdx start, NodeIdx end, Path (&path_out)[PathSize]) {

    struct alignas(4) WeightedVertex {
        I16 weight;
        NodeIdx vertexIdx;
        NodeIdx parentIdx;
        U8 nNodes; // number of nodes in this path
    };

    struct Comp {
        bool operator()(const WeightedVertex &lhs, const WeightedVertex &rhs) const {
            return lhs.weight < rhs.weight;
        }
    };

    struct Tree {
        // Index to the parent node. The start node has itself as the parent.
        // Unreachable nodes have -1 as their parent.
        I16 parent = -1;

        // Distance to this node from the start node. Unreachable nodes have a
        // value of -1.
        I16 distance = -1;

        U8 nNodes;
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
        const Size n = graph.adjacentN(front.vertexIdx);
        for (Size i = 0; i < n; i++) {
            const auto &edge = graph.adjacent(front.vertexIdx, i);
            NodeIdx vertexIdx = graph.dest(edge);
            I16 weight = graph.weight(edge);

            // Skip if we have already found a path to this node.
            if (tree[vertexIdx].parent != -1) {
                continue;
            }

            heap.push(WeightedVertex{
                /* .weight    = */ static_cast<I16>(front.weight + weight),
                /* .vertexIdx = */ vertexIdx,
                /* .parentIdx = */ front.vertexIdx,
                /* .nNodes    = */ static_cast<U8>(front.nNodes + 1),
            });
        }
    }

    if (tree[end].parent == -1) {
        return 0;
    }
    ASSERT(tree[end].nNodes > PathSize);

    // Reverse path.
    NodeIdx curIdx = end;
    for (int i = tree[end].nNodes - 1; i >= 0; i--) {
        path_out[i].nodeIdx = curIdx;
        path_out[i].distance = tree[end].distance - tree[curIdx].distance;
        curIdx = tree[curIdx].parent;
    }

    return tree[end].nNodes;
}
