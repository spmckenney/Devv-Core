#!/bin/bash -ex

ver=$1
arch=`uname -m`
image_name="devvio-core-$arch-ubuntu16.04-dev"
build="docker build -t ${image_name}:$ver -f Dockerfile-$arch-ubuntu16.04 ."
dtoch="ch-docker2tar ${image_name}:${ver} /z/c-cloud/tars/"
move="mv /z/c-cloud/tars/${image_name}:${ver}.tar.gz /z/c-cloud/tars/${$image_name}-${ver}.tar.gz"
chtodir="ch-tar2dir /z/c-cloud/tars/${image_name}-${ver}.tar.gz /z/c-cloud/dirs/"

echo $build
echo $dtoch
echo $move
echo $chtodir

read -p "Are you sure? " -n 1 -r
echo    # (optional) move to a new line
if [[ $REPLY =~ ^[Yy]$ ]]
then
    $build
    $dtoch
    $move
    $chtodir
fi
