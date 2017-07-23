#pragma once

#include "track_node.h"
#include "track_data_new.h"
#include "std.h"

void cmdSetStoppingDistance(int mm);
void cmdClearBrokenSwitches();

const TrackData &Track();

struct Graph {
    const TrackNode *vertices;

    static constexpr Size VSize = TRACK_MAX;

    auto adjacentN(int idx) const {
        switch (vertices[idx].type) {
            case NODE_BRANCH: return 2;
            case NODE_MERGE: return 1;
            case NODE_SENSOR:
            case NODE_BROKEN_SENSOR:
                              return 1;
            case NODE_BROKEN_SW_ST:
            case NODE_BROKEN_SW_CV:
                              return 1;
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
    U8 reverse(int idx) const {
        return vertices[idx].reverse - vertices;
    }
    bool isSwitch(int idx) const {
        return vertices[idx].type == NODE_MERGE
            || vertices[idx].type == NODE_BRANCH;
    }
    int getSwitchNum(int idx) const {
        return vertices[idx].num;
    }
    char getSwitchPath(int childIdx, int idx) const {
        if (vertices[idx].type == NODE_MERGE)
            return 'N';
        if (vertices[idx].edge[DIR_STRAIGHT].dest == &vertices[childIdx])
            return 'S';
        if (vertices[idx].edge[DIR_CURVED].dest == &vertices[childIdx])
            return 'C';
        return 'X';
    }
};
