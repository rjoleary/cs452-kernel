#pragma once

#include "track_node.h"
#include "track_data_new.h"
#include "std.h"

void trackMain();

void cmdRoute(int train, int speed, int sensor);

struct Graph {
    TrackNode *vertices;

    static constexpr ctl::size_t VSize = TRACK_MAX;

    auto adjacentN(int idx) const {
        switch (vertices[idx].type) {
            case NODE_SENSOR: return 1;
            case NODE_BRANCH: return 2;
            case NODE_MERGE: return 1;
            case NODE_ENTER: return 1;
            case NODE_EXIT: return 0;
            default: return 0; // TODO: make enum class
        }
    }
    const TrackEdge *adjacent(int idx, int i) const {
        return &(vertices[idx].edge[i]);
    }
    auto dest(const TrackEdge *edge) const {
        return edge->dest - vertices;
    }
    auto weight(const TrackEdge *edge) const {
        return edge->dist;
    }
};
