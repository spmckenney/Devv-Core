#!/bin/bash

# ../scripts/run_devcash_node.sh ${NODE_INDEX} 2>&1 | tee /home/spmckenney/dmnt/logs/dc_out_$(cat /home/spmckenney/dmnt/logs/log_inc.txt)_${NODE_INDEX}.log

node_index=$1

if [ "${node_index}x" = "x" ]; then
    if [ "${NODE_INDEX}x" = "x" ]; then
        echo "SET A NODE INDEX (either \$NODE_INDEX or \$1)"
        exit -1
    else
        node_index=${NODE_INDEX}
    fi
fi

update_log_index=""

if [ ${node_index} -eq 2 ]; then
    echo "we be threee!"
    update_log_index="(node 3 - updating log counter)"
    log_num=$(/home/spmckenney/dmnt/logs/log_inc.sh)
else
    echo "no three for me"
    log_num=$(cat /home/spmckenney/dmnt/logs/log_inc.txt)
fi

echo "node index: ${node_index} $update_log_index"

run_script=${HOME}/dmnt/devcash-core/scripts/run_devcash_node.sh

cmd="$run_script ${node_index} 2>&1 | tee /home/spmckenney/dmnt/logs/dc_out_${log_num}_${node_index}.log"

echo $cmd

$cmd
