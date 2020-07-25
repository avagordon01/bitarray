#!/usr/bin/env bash
if [ ! -d subprojects ]; then
    meson wrap install gtest
fi
if [ ! -d out ]; then
    CXX=clang++ \
    meson out
fi
meson test -C out --print-errorlogs
