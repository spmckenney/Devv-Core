#!/bin/bash -ex

cd ${HOME}
mkdir -p src
cd src

git clone https://github.com/google/googletest.git

cd googletest
git checkout release-1.8.0

mkdir build
cd build
cmake ..
make
sudo make install
cd ..
mv googletest /usr/src/
mv googlemock /usr/src/gmock
