#!/bin/bash
rm -rf bench && mkdir bench
cd build && make -j$(nproc) && cd ..
perf record -g -- ./build/termfm
perf report --stdio > bench/perf_report.txt
open bench/perf_report.txt