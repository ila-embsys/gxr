#!/bin/bash

mkdir -p deps
cd deps

git clone https://gitlab.freedesktop.org/xrdesktop/gulkan.git
cd gulkan
git checkout $1
meson build --prefix /usr
ninja -C build install
cd ../..
