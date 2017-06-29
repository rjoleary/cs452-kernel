#pragma once

enum NodeType {
    NODE_SENSOR,
    NODE_BRANCH,
    NODE_MERGE,
    NODE_ENTER,
    NODE_EXIT,
    NODE_BROKEN_SENSOR,
    NODE_BROKEN_SW_ST,
    NODE_BROKEN_SW_CV
};

constexpr auto DIR_AHEAD = 0;
constexpr auto DIR_STRAIGHT =  0;
constexpr auto DIR_CURVED = 1;

struct TrackNode;
struct TrackEdge {
    TrackEdge *reverse;
    TrackNode *src, *dest;
    int dist;             /* in millimetres */
};

struct TrackNode {
    const char *name;
    NodeType type;
    int num;              /* sensor or switch number */
    TrackNode *reverse;  /* same location, but opposite direction */
    TrackEdge edge[2];
};
