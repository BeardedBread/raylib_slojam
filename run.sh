#!/bin/sh

cd build/ && make -j4 && cd .. && LSAN_OPTIONS=suppressions=./lsan_supp.txt  ./build/slojam
