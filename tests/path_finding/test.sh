#!/bin/bash
set -e

clang++ -Wall -Werror -std=c++14 -g -I../../include/user dijkstra_test.cpp ../../src/track/track_data_new.cpp -o dijkstra
gdb ./dijkstra || true
rm dijkstra
