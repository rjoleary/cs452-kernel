// Tests that dijkstra's algorithm on a few small test cases.
#include <vector>
#include <iostream>
#include <path_finding.h>
#include <track.h>

using namespace ctl;

namespace ctl {
    void assert(const char *) {
        std::abort();
    }
}

int main() {
    auto track = init_tracka();

    auto ss = dijkstra(Graph{track.nodes}, 45);

    for (const auto &sse : ss.states) {
        std::cout << "Switch state: " << sse<<"\n";
    }
}
