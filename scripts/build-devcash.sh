#!/bin/bash

# Build script for devcash, assumes the executing party is cd'd into
# the <root repository path>/scripts before running this command. This is
# required because we don't know which absolute path the script will reside in.
# -Andree, 4/11/2018

repo=devcash-core
container_version=`cat ../.container_version`

# Get a node to build on
salloc -N 1

echo "Pulling repo"
mkdir -p ~/Devel/${SLURM_NODELIST}
cd ~/Devel/${SLURM_NODELIST}
if [ -d $repo ]; then
  echo "Found repo, pulling changes"
  pushd $repo
else
  echo "Repo nonexistent, cloning"
  git clone git@bitbucket.org:leveragerock/devcash-core.git
fi
git checkout $1
git pull
cd ../..

docker_container=x86_64-ubuntu16.04-devcash-v${container_version}

# Unpack the tarball for the build image onto the ramdisk (using this to build)
ssh $SLURM_NODELIST /opt/local/bin/ch-tar2dir /z/c-cloud/tars/${docker_container}.tar.gz /mnt/ramdisk

# Now, run the build script
ssh $SLURM_NODELIST /opt/local/bin/ch-run /mnt/ramdisk/${docker_container} -- sh -c "cd /home/bitbucket/Devel/${SLURM_NODELIST}/devcash-core/src; mkdir -p build; cd build; cmake ..; make -j 16"

exit

