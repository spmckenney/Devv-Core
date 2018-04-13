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

docker_container=x86_64-ubuntu16.04-devcash-v${container_version}

# Unpack the tarball for the build image onto the ramdisk (if it's not already there)
ssh ${build_host} test -d /mnt/ramdisk/${docker_container}
if [ $? -eq 1 ]; then
  # Only unpack if not already there, we won't be able to remove it anyway (unless we put it there)
  srun /opt/local/bin/ch-tar2dir /z/c-cloud/tars/${docker_container}.tar.gz /mnt/ramdisk
fi
echo "Updating group ownership and permissions"
srun /usr/bin/chgrp -R charlie /mnt/ramdisk/${docker_container} 2>&1 > /dev/null
srun /usr/bin/chmod -R g+w /mnt/ramdisk/${docker_container} 2>&1 > /dev/null

# Now, run the build script
srun /opt/local/bin/ch-run /mnt/ramdisk/${docker_container} -- sh -c "cd ~/Devel/${build_host}/${repo}/src; mkdir -p build; cd build; cmake ..; make -j $build_cores"

# If we can, remove the image...
echo "Trying to remove ramdisk image, if possible..."
srun /usr/bin/rm -rf /mnt/ramdisk/${docker_container} 2>&1 > /dev/null
