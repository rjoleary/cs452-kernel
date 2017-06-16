#!/bin/sh
set -e

make clean && make OPT_ENABLED=0 CACHE_ENABLED=0
make clean && make OPT_ENABLED=0 CACHE_ENABLED=1
make clean && make OPT_ENABLED=1 CACHE_ENABLED=0
make clean && make OPT_ENABLED=1 CACHE_ENABLED=1

echo All made successfully!
