#!/bin/bash
rm -rf bench && mkdir bench
rm perf.data
cd build && make -j$(nproc) && cd ..
perf record -g -- ./build/termfm
perf script > bench/perf.script
