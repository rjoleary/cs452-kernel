// Tests that dijkstra's algorithm on a few small test cases.
#include <vector>
#include <iostream>
#include <path_finding.h>
#include <track.h>

using namespace ctl;

namespace ctl {
    void assert(const char *) {
    }
}

int main() {
    TrackNode nodes[TRACK_MAX];
    init_tracka(nodes);

    Path out[TRACK_MAX];
    int n = dijkstra(Graph{nodes}, 10, 50, out);

    for (int i = 0; i < n; i++) {
        std::cout << "node: " << out[i].nodeIdx << ", dist: " << out[i].distance << '\n';
    }
}
