/* THIS FILE IS GENERATED CODE -- DO NOT EDIT */
#pragma once

#include "track_node.h"

// The track initialization functions expect an array of this size.
#define TRACK_MAX 144

struct TrackData {
    TrackNode nodes[TRACK_MAX];
};

TrackData init_tracka();
//void init_trackb(TrackNode *track);
