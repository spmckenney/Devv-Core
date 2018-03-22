#!/bin/bash -ex

set -ex

git clone https://github.com/facebook/fbzmq.git

cd fbzmq
git checkout -b v2018.03.05.00_branch v2018.03.05.00
patch -p1 < ../fbzmq_docker_build.patch
cd build
bash ./build_fbzmq.sh
