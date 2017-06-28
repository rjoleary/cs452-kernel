#!/bin/bash
set -e

clang++ -Wall -Werror -std=c++14 -I../include/user dijkstra_test.cpp -o dijkstra
./dijkstra || true
rm dijkstra
