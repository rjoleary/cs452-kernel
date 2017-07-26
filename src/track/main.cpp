#include <track.h>

#include <track_node.h>
#include <track_data_new.h>

const TrackData &Track() {
    static auto trackData = init_tracka();
    return trackData;
}

void cmdClearBrokenSwitches() {
    // TODO: repurpose
}
