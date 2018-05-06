#!/usr/bin/python3

import sys
import os
import time
import subprocess

#nodes=['node0', 'node1', 'node2']
nodes=['localhost', 'localhost', 'localhost']

sync_host="localhost"
mode="T2"
debug_mode="off"
num_threads=1
proto="tcp"
base_port = 56550
trace_file="trace_file"
num_transactions=100000
tx_batch_size=2000

log_dir = sys.argv[1]

if len(nodes) < 3:
    print("Error, number of nodes must be >= 3")

bind_port = []
node_target = []
for i in range(len(nodes)):
    bind_port.append(proto+"://*:"+str(base_port+i))
    node_target.append(proto+"://"+nodes[i]+":"+str(base_port+i))


host_list = []
for i in range(len(nodes)):
    host_list.append([])
    for index, node in enumerate(nodes):
        if (index == i):
            continue
        host_list[i].append(node_target[index])


cmds = []
for i in range(len(nodes)):
    cmds.append([])
    cmd = cmds[i]
    #cmd.append("ch-run")
    #cmd.append("/z/c-cloud/dirs/x86_64-ubuntu16.04-devcash-v021")
    #cmd.append("--")
    cmd.append("devcash")
    cmd.extend(["--node-index", str(i)])
    cmd.extend(["--debug-mode", debug_mode])
    cmd.extend(["--mode", mode])
    cmd.extend(["--num-consensus-threads", str(num_threads)])
    cmd.extend(["--num-validator-threads", str(num_threads)])
    for host in host_list[i]:
        cmd.extend(["--host-list", host])
    cmd.extend(["--sync-host", sync_host])
    cmd.extend(["--output", os.path.join(log_dir,"output_"+str(i)+".dat")])
    cmd.extend(["--trace-output", os.path.join(log_dir,"trace_"+str(i)+".json")])
    cmd.extend(["--generate-tx", str(num_transactions)])
    cmd.extend(["--tx-batch-size", str(tx_batch_size)])
    cmd.extend(["--bind-endpoint", bind_port[i]])

cmds.append(["synchronize_devcash_nodes.py", "3"])

#############################

ps = []
for cmd in cmds:
    print(cmd)
    ps.append(subprocess.Popen(cmd))
    time.sleep(1.5)

for p in ps:
    print("Waiting...")
    p.wait()


print("Done")
