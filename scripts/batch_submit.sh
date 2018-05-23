#!/bin/bash -ex

run=$1
num_submit=$2

rundir=${HOME}/run-$run
mkdir -p $rundir
cd $rundir

for i in `seq 1 $num_submit`; do
    cmd="sbatch ${HOME}/dmnt/devcash-core/scripts/batch_run.sh $run $i"
    echo $cmd
    $cmd
done
