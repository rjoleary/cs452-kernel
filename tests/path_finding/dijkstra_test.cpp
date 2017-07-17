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

    auto ss = dijkstra(Graph{nodes}, 66);

    for (const auto &sse : ss.states) {
        std::cout << "Switch state: " << sse<<"\n";
    }
}
