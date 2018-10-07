#!/bin/bash -ex

ver=$1
git_branch=$2

## Build dev and prod
declare -a image_types=("dev" "prod")

## now loop through the types and build
for image_type in "${image_types[@]}"
do
    docker build \
	   --build-arg version=${ver} \
	   --build-arg branch=${git_branch} \
	   -t devvio-x86_64-ubuntu16.04-${image_type}:${ver} \
	   -f Dockerfile-x86_64-ubuntu16.04-${image_type} .
done
