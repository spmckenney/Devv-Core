#!/usr/bin/python3

import sys

bind_port_prefix = 55550


nodes = []

for n in range(3):
    node_def = {}
    node_def['node-index'] = n
    node_def['debug-mode'] = "toy"
    node_def['mode'] = "T1"
    node_def['bind-port'] = bind_port_prefix + n

    node_def['ip'] = 1
    node_def['threads'] = 1


    nodes.append(node_def)



node_num = int(sys.argv[1])

node=1

ss = "./devcash --node-index "+str(node)+\
     " --debug-mode
