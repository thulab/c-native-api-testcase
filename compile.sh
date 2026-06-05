#!/bin/bash
# 编译脚本

if [ -d "build" ]; then
    rm -rf build/*
    rmdir build/* 2>/dev/null
else
    mkdir build
fi

cd build
cmake ..
make
