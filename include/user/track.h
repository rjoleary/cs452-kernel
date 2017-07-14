#pragma once

#include "track_node.h"
#include "track_data_new.h"
#include "std.h"

void cmdRoute(int train, int speed, int sensor);
void cmdSetStoppingDistance(int mm);
void cmdClearBrokenSwitches();

struct Graph {
    TrackNode *vertices;

    static constexpr Size VSize = TRACK_MAX;

    auto adjacentN(int idx) const {
        switch (vertices[idx].type) {
            case NODE_BRANCH: return 2;
            case NODE_SENSOR:
            case NODE_MERGE:
            case NODE_BROKEN_SENSOR:
            case NODE_BROKEN_SW_ST:
            case NODE_BROKEN_SW_CV:
            case NODE_ENTER: return 1;
            case NODE_EXIT: return 0;
            default: return 0; // TODO: make enum class
        }
    }
    const TrackEdge *adjacent(int idx, int i) const {
        if (vertices[idx].type == NODE_BROKEN_SW_CV)
            return &vertices[idx].edge[i+1];
        return &vertices[idx].edge[i];
    }
    auto dest(const TrackEdge *edge) const {
        return edge->dest - vertices;
    }
    auto weight(const TrackEdge *edge) const {
        return edge->dist;
    }
};
