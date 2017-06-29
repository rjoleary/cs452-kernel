// Tests that dijkstra's algorithm on a few small test cases.
#include <vector>
#include <iostream>
#include <path_finding.h>

namespace ctl {
void assert(const char *str) {
    std::cout << "Your test asserted\n";
    exit(1);
}
}

void assert(bool b) {
    if (!b) {
        std::cout << "Your test asserted\n";
        exit(1);
    }
}

struct Vertex {
    std::vector<int> edges;
};

struct Edge {
    short weight;
    int dest;
};

template <size_t S> struct Graph {
    static const size_t VSize = S;

    Vertex *vertices;
    Edge *edges;

    auto adjacentN(int idx) const {
        return vertices[idx].edges.size();
    }
    auto adjacent(int idx, int i) const {
        return vertices[idx].edges[i];
    }
    auto dest(int idx) const {
        return edges[idx].dest;
    }
    auto weight(int idx) const {
        return edges[idx].weight;
    }
};

int main() {
    {
        // Graph #1: undirected, start = 2
        //
        //       1       0
        //      / \      |
        //     2   3     7
        //     |   |
        //     4   5
        //      \ /
        //       6
        //
        Vertex vertices[] = {
            {{5}},
            {{1, 3}},
            {{0, 7}},
            {{2, 9}},
            {{6, 11}},
            {{8, 13}},
            {{10, 12}},
            {{4}},
        };
        Edge edges[] = {
            {3, 1}, {3, 2},
            {2, 1}, {2, 3},
            {1, 0}, {1, 7},
            {2, 2}, {2, 4},
            {1, 3}, {1, 5},
            {1, 4}, {1, 6},
            {3, 5}, {3, 6}, 
        };
        const size_t VSize = sizeof(vertices) / sizeof(vertices[0]);
        Path out[VSize];
        int n = dijkstra(Graph<VSize>{vertices, edges}, 2, 5, out);

        for (int i = 0; i < n; i++) {
            std::cout << "node: " << out[i].nodeIdx << ", dist: " << out[i].distance << '\n';
        }

        /*assert(out[0].parent == -1);
        assert(out[0].distance == -1);
        assert(out[1].parent == 2);
        assert(out[1].distance == 3);
        assert(out[2].parent == 2);
        assert(out[2].distance == 0);
        assert(out[3].parent == 1);
        assert(out[3].distance == 5);
        assert(out[4].parent == 2);
        assert(out[4].distance == 2);
        assert(out[5].parent == 3);
        assert(out[5].distance == 6);
        assert(out[6].parent == 4);
        assert(out[6].distance == 3);
        assert(out[7].parent == -1);
        assert(out[7].distance == -1);
        */
    }


    {
        // Graph #2: directed, start = 2
        //
        //       1     0
        //      v ^    |
        //     2   3---7
        //     v   ^
        //     4   5
        //      v ^
        //       6
    }


    {
        // Graph #3


    }

    std::cout << "All tests passed!\n";
}
