#!/bin/bash

cd ${HOME}
mkdir -p src
cd src

git clone https://github.com/google/flatbuffers.git

cd flatbuffers
git checkout v1.9.0
cmake -DCMAKE_INSTALL_PREFIX=/usr/local
make -j4 && sudo make install

exit 0
