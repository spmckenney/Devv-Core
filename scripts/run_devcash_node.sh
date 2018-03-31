#!/bin/bash

node=$1

mode="T1"
debug_mode="toy"
num_threads=1
proto="tcp"

node0_ip="127.0.0.1"
node1_ip="127.0.0.1"
node2_ip="127.0.0.1"

node0_port="55550"
node1_port="55551"
node2_port="55552"

bind_port[0]="${proto}://*:${node0_port}"
bind_port[1]="${proto}://*:${node1_port}"
bind_port[2]="${proto}://*:${node2_port}"

node0_target="${proto}://${node0_ip}:${node0_port}"
node0_target="${proto}://${node0_ip}:${node0_port}"
node0_target="${proto}://${node0_ip}:${node0_port}"

node_index=(0 1 2)

case $node in
    0)
        echo "setting up node 0"
        hostA[$node]="tcp://127.0.0.1:55551"
        hostB[$node]="tcp://127.0.0.1:55552"
        ;;
    1)
        echo "setting up node 1"
        hostA[$node]="tcp://127.0.0.1:55550"
        hostB[$node]="tcp://127.0.0.1:55552"
        ;;
    2)
        echo "setting up node 2"
        hostA[$node]="tcp://127.0.0.1:55550"
        hostB[$node]="tcp://127.0.0.1:55551"
        ;;
    *)
        echo "Error - unknown node"
        ;;

esac

cmd="./devcash --node-index ${node} \
--debug-mode off \
--mode T1 \
--num-consensus-threads 1 \
--num-validator-threads 1 \
--scan-file /home/dcrunner/dmnt/devcash-core/opt/1000txs
--host-list ${hostA[$node]} \
--host-list ${hostB[$node]} \
--bind-endpoint ${bind_port[$node]}"

echo $cmd
$cmd
