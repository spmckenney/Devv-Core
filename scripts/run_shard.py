#!/usr/bin/python3

import sys
import os
import time
import subprocess

#nodes=['node0', 'node1', 'node2']
nodes=['localhost',
       'localhost',
       'localhost',
       'localhost',
       'localhost',
       'localhost',
       'localhost',
       'localhost',
       'localhost']

sync_host="devcash"
nodes_per_shard=3
num_t2_shards=2
debug_mode="off"
num_threads=1
proto="tcp"
base_port = 56550
sync_base_port = sys.argv[2]
trace_file="trace_file"
num_t2_transactions=1000
tx_batch_size=20
gdb="gdb -ex run --args"
#scan_dir="/home/spmckenney/run-14/shard-1"
scan_dir="/home/spmckenney/dmnt/nicks/r1"
key_dir= "/home/spmckenney/dmnt/nicks/keys"

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
    shard_index=int(i / nodes_per_shard)
    node_index=int(i % nodes_per_shard)
    for index, node in enumerate(nodes):
        # skip self
        if (index == i):
            continue
        # skip when neither shards nor indices match
        if ((shard_index != int(index / nodes_per_shard))
            and (node_index != int(index % nodes_per_shard))):
            continue
        # skip if both shards are T2 and different shards
        if ((shard_index > 0) and (int(index / nodes_per_shard) > 0)):
            if (shard_index != int(index / nodes_per_shard)):
                continue
        host_list[i].append(node_target[index])


cmds = []
for i in range(len(nodes)):
    shard_index = int(i / nodes_per_shard)
    cmds.append([])
    cmd = cmds[i]
    #cmd.append("ch-run")
    #cmd.append("/z/c-cloud/dirs/x86_64-ubuntu16.04-devcash-v021")
    #cmd.append("--")
    cmd.append(gdb + " ./devcash")
    cmd.extend(["--node-index", str(i % nodes_per_shard)])
    cmd.extend(["--shard-index", str(shard_index)])
    cmd.extend(["--debug-mode", debug_mode])
    cmd.extend(["--mode", "T2" if i > 2 else "T1"])
    cmd.extend(["--num-consensus-threads", str(num_threads)])
    cmd.extend(["--num-validator-threads", str(num_threads)])
    for host in host_list[i]:
        cmd.extend(["--host-list", host])
    cmd.extend(["--sync-host", sync_host])
    cmd.extend(["--sync-port", int(sync_base_port)+i])
    cmd.extend(["--output", os.path.join(log_dir,"output_"+str(i)+".dat")])
    cmd.extend(["--trace-output", os.path.join(log_dir,"trace_"+str(i)+".json")])
    if shard_index > 0:
        cmd.extend(["--generate-tx", str(num_t2_transactions)])
        cmd.extend(["--tx-batch-size", str(tx_batch_size)])
        cmd.extend(["--tx-limit", str(num_t2_transactions * num_t2_shards * nodes_per_shard)])
    else:
        cmd.extend(["--scan-dir", scan_dir])
        cmd.extend(["--tx-limit", str(100)])
    cmd.extend(["--bind-endpoint", bind_port[i]])
    cmd.extend(["--inn-keys", os.path.join(key_dir,"inn.key")])
    cmd.extend(["--node-keys", os.path.join(key_dir,"node.key")])
    cmd.extend(["--wallet-keys", os.path.join(key_dir,"wallet.key")])

cmds.append(["synchronize_devcash_nodes.py", "3"])

#############################

ps = []
for cmd in cmds:
    #for i in cmd:
    #    print("{} ".format(i))
    print("")
    print(*cmd)
    #ps.append(subprocess.Popen(cmd))
    #time.sleep(1.5)

exit(0)

for p in ps:
    print("Waiting...")
    p.wait()


print("Done")
