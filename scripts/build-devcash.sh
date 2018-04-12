#!/bin/bash

# Build script for devcash, intended to be launched through SLURM via
#   salloc -N 1 <path to script>/build-devcash.sh <branch>

# Name of devcash repository to check out
repo=devcash-core

# Find root directory of the script location
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Read current container version from config file (at root of repo)
container_version=`cat ${script_dir}/../.container_version`

# Find hostname we're going to build on
build_host=$SLURM_NODELIST

# And the number of cores / threads on that node (as SLURM reports)
build_cores=$SLURM_TASKS_PER_NODE

# Grab the branch name from the command line
build_branch=$1

# While still on master node, pull repo and checkout correct branch
# (passed as argument to script)
echo "Pulling repo"
mkdir -p ~/Devel/$build_host
cd ~/Devel/$build_host
if [ -d $repo ]; then
  echo "Found repo, pulling changes"
  pushd $repo
else
  echo "Repo nonexistent, cloning"
  git clone git@bitbucket.org:leveragerock/${repo}.git
fi
git checkout $build_branch
git pull
cd ../..

docker_container=x86_64-ubuntu16.04-devcash-v${container_version}

# Unpack the tarball for the build image onto the ramdisk (using this to build)
ssh $build_host \
  /opt/local/bin/ch-tar2dir /z/c-cloud/tars/${docker_container}.tar.gz /mnt/ramdisk

# Now, run the build script 
ssh $build_host \
  /opt/local/bin/ch-run /mnt/ramdisk/${docker_container} -- \
  sh -c "cd ~/Devel/${build_host}/${repo}/src; mkdir -p build; cd build; cmake ..; make -j $build_cores"
