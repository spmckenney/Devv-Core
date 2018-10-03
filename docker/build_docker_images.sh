#!/bin/bash -ex

ver=$1
branch=$2

## Build dev and prod
declare -a image_types=("dev" "prod")

## now loop through the types and build
for image_type in "${image_types[@]}"
do
    docker build \
	   --build-arg version=${ver} \
	   --build-arg branch=${branch} \
	   -t devvio-x86_64-ubuntu16.04-${image_type}:${ver} \
	   -f Dockerfile-x86_64-ubuntu16.04-${image_type} .
done
