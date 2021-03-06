#pragma once

#include "heap.h"
#include "std.h"
#include "switch.h"
#include "track.h"

using ctl::Heap;

typedef U8 NodeIdx;
constexpr NodeIdx INVALID_NODE = 255;

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
SwitchStates dijkstra(const T &graph, NodeIdx end) {
    struct alignas(4) WeightedVertex {
        I16 weight;
        NodeIdx vertexIdx;
        NodeIdx childIdx;
    };

    struct Comp {
        bool operator()(const WeightedVertex &lhs, const WeightedVertex &rhs) const {
            return lhs.weight < rhs.weight;
        }
    };

    struct Tree {
        // Index to the child node. The start node has itself as the child.
        // Unreachable nodes have -1 as their child.
        I16 child    = -1;

        // Distance to this node from the start node. Unreachable nodes have a
        // value of -1.
        I16 distance = -1;
    } tree[T::VSize];

    // Push the first node onto the heap.
    Heap<T::VSize, WeightedVertex, Comp> heap;
    heap.push(WeightedVertex{
        /* .weight    = */ 0,
        /* .vertexIdx = */ end,
        /* .childIdx  = */ end
    });

    SwitchStates ss;

    while (!heap.empty()) {
        // Pop the shortest path from the heap.
        WeightedVertex front = heap.pop();

        if (tree[front.vertexIdx].child != -1
                || tree[graph.reverse(front.vertexIdx)].child != -1) {
            continue;
        }

        // Update the weight and child.
        tree[front.vertexIdx] = Tree{
            /* .child    = */ front.childIdx,
            /* .distance = */ front.weight
        };
        if (graph.isSwitch(front.vertexIdx)) {
            ss[graph.getSwitchNum(front.vertexIdx)] =
                graph.getSwitchPath(front.childIdx, front.vertexIdx);
        }

        // Push all the adjacent nodes onto the heap.
        for (int i = 0; i < graph.adjacentN(front.vertexIdx); i++) {
            const auto &edge = graph.adjacent(front.vertexIdx, i);
            NodeIdx vertexIdx = graph.reverse(graph.dest(edge));
            I16 weight = graph.weight(edge) + 500;

            // Skip if we have already found a path to this node.
            if (tree[vertexIdx].child != -1
                    || tree[graph.reverse(vertexIdx)].child != -1) {
                continue;
            }

            heap.push(WeightedVertex{
                /* .weight    = */ I16(front.weight + weight),
                /* .vertexIdx = */ vertexIdx,
                /* .childIdx  = */ graph.reverse(front.vertexIdx)
            });
        }
        for (int i = 0 ; i < graph.adjacentN(graph.reverse(front.vertexIdx)); i++) {
            const auto &edge = graph.adjacent(graph.reverse(front.vertexIdx), i);
            NodeIdx vertexIdx = graph.dest(edge);
            I16 weight = graph.weight(edge);

            // Skip if we have already found a path to this node.
            if (tree[vertexIdx].child != -1
                    || tree[graph.reverse(vertexIdx)].child != -1) {
                continue;
            }

            heap.push(WeightedVertex{
                /* .weight    = */ I16(front.weight + weight),
                /* .vertexIdx = */ graph.reverse(vertexIdx),
                /* .childIdx  = */ front.vertexIdx
            });
        }
    }
    return ss;
}

template <Size MaxSize>
Distance shortestDistance(const TrackData &graph, NodeIdx start, NodeIdx end) {
    struct alignas(4) WeightedVertex {
        I16 weight;
        NodeIdx vertexIdx;
    };

    struct Comp {
        bool operator()(const WeightedVertex &lhs, const WeightedVertex &rhs) const {
            return lhs.weight < rhs.weight;
        }
    };

    struct Tree {
        // Distance to this node from the start node. Unreachable nodes have a
        // value of -1.
        I16 distance = -1;

        U8 nNodes;
    } tree[MaxSize];

    // Push the first node onto the heap.
    Heap<MaxSize, WeightedVertex, Comp> heap;
    heap.push(WeightedVertex{
        /* .weight    = */ 0,
        /* .vertexIdx = */ start,
    });

    while (!heap.empty()) {
        // Pop the shortest path from the heap.
        WeightedVertex front = heap.pop();

        if (tree[front.vertexIdx].distance != -1) continue;

        // Update the weight and parent.
        tree[front.vertexIdx] = Tree{
            /* .distance = */ front.weight,
        };

        // Exit early if we reached the end node.
        if (front.vertexIdx == end) {
            break;
        }

        // Determine number of adjacent nodes.
        Size adjacentN = 0;
        switch (graph.nodes[front.vertexIdx].type) {
			case NODE_BRANCH:
				adjacentN = 2;
				break;
			case NODE_SENSOR:
			case NODE_MERGE:
			case NODE_BROKEN_SENSOR:
			case NODE_BROKEN_SW_ST:
			case NODE_BROKEN_SW_CV:
			case NODE_ENTER:
				adjacentN = 1;
				break;
			case NODE_EXIT:
                adjacentN = 0;
                break;
		}

        // Push all the adjacent nodes onto the heap.
        for (Size i = 0; i < adjacentN; i++) {
            const auto &edge = graph.nodes[front.vertexIdx].type == NODE_BROKEN_SW_CV ?
                &graph.nodes[front.vertexIdx].edge[i+1] :
                &graph.nodes[front.vertexIdx].edge[i];
            NodeIdx vertexIdx = edge->dest - graph.nodes;
            I16 weight = edge->dist;

            // Skip if we have already found a path to this node.
            if (tree[vertexIdx].distance != -1) {
                continue;
            }

            heap.push(WeightedVertex{
                /* .weight    = */ static_cast<I16>(front.weight + weight),
                /* .vertexIdx = */ vertexIdx,
            });
        }
    }

    if (tree[end].distance == -1) {
        return 0;
    }

    return tree[end].distance;
}
