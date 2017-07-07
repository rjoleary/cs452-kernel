#pragma once

#include "track_node.h"
#include "track_data_new.h"
#include "std.h"

void trackMain();

void cmdRoute(int train, int speed, int sensor);
void cmdSetStoppingDistance(int mm);
void cmdClearBrokenSwitches();

