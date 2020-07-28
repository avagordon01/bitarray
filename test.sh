#!/usr/bin/env bash
if [ ! -d subprojects ]; then
    meson wrap install gtest
fi
if [ ! -d out-clang ]; then
    CXX=clang++ \
    meson out-clang
fi
if [ ! -d out-gcc ]; then
    CXX=g++ \
    meson out-gcc
fi
meson test -C out-gcc --print-errorlogs
meson test -C out-clang --print-errorlogs
