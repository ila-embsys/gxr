#!/bin/bash

mkdir -p deps
cd deps

git clone --branch $1 https://github.com/KhronosGroup/OpenXR-SDK.git
cd OpenXR-SDK
cmake . \
  -G Ninja \
  -B build \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=OFF \
  -DDYNAMIC_LOADER=ON \
  -DBUILD_WITH_STD_FILESYSTEM=OFF
ninja -C build install
cd ../..
