#include <track.h>

#include <track_node.h>
#include <track_data_new.h>

const TrackData &Track() {
    static auto trackData = init_tracka();
    return trackData;
}

void cmdSetStoppingDistance(int ) {
    // TODO: repurpose
}

void cmdClearBrokenSwitches() {
    // TODO: repurpose
}
