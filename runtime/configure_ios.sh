#!/bin/bash

if [[ "$OSTYPE" == "darwin"* ]]; then
    BUILD_DIR="build/ios"
    PLATFORM="iOS"
else
    echo "Unsupported platform: $OSTYPE"
    exit 1
fi

mkdir -p $BUILD_DIR
cd $BUILD_DIR

echo "Configuring for $PLATFORM..."
cmake -S ../.. -B . -G Xcode -DCMAKE_TOOLCHAIN_FILE=../../ios.toolchain.cmake -DPLATFORM=OS64 -DENABLE_ARC=FALSE

echo "Complete! Xcode project is located in $BUILD_DIR"