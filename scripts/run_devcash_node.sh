#!/bin/bash

node=$1

# export DEVCASH_OUTPUT_LOGLEVEL=debug
# export DEVCASH_OUTPUT_LOGLEVEL=info
# export DEVCASH_OUTPUT_LOGLEVEL=warning

mode="T1"
debug_mode="toy"
num_threads=1
proto="tcp"
scan_file="${HOME}/dmnt/devcash-core/opt/05txs"
#scan_file="${HOME}/dmnt/devcash-core/opt/21txs"
#scan_file="${HOME}/dmnt/devcash-core/opt/100txs"
#scan_file="${HOME}/dmnt/devcash-core/opt/1000txs"
#scan_file="${HOME}/dmnt/devcash-core/opt/1002txs"
#scan_file="${HOME}/dmnt/devcash-core/opt/10k.txs"
#scan_file="${HOME}/dmnt/devcash-core/opt/20k_txs"

node0_ip="dc001"
node1_ip="dc002"
node2_ip="dc003"

node0_port="55550"
node1_port="55551"
node2_port="55552"

bind_port[0]="${proto}://*:${node0_port}"
bind_port[1]="${proto}://*:${node1_port}"
bind_port[2]="${proto}://*:${node2_port}"

node0_target="${proto}://${node0_ip}:${node0_port}"
node1_target="${proto}://${node1_ip}:${node1_port}"
node2_target="${proto}://${node2_ip}:${node2_port}"

node_index=(0 1 2)

case $node in
    0)
        echo "setting up node 0"
        hostA[$node]=${node1_target}
        hostB[$node]=${node2_target}
        ;;
    1)
        echo "setting up node 1"
        hostA[$node]=${node0_target}
        hostB[$node]=${node2_target}
        ;;
    2)
        echo "setting up node 2"
        hostA[$node]=${node0_target}
        hostB[$node]=${node1_target}
        ;;
    *)
        echo "Error - unknown node"
        ;;

esac

cmd="./devcash --node-index ${node} \
--debug-mode off \
--mode T2 \
--num-consensus-threads 10 \
--num-validator-threads 10 \
--scan-file ${scan_file} \
--host-list ${hostA[$node]} \
--host-list ${hostB[$node]} \
--output output_${node}.out \
--bind-endpoint ${bind_port[$node]}"

echo $cmd
$cmd
