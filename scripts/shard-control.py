#!/usr/bin/python3

import sys
sys.path.append("/home/mckenney/devcash/devcash-core/scripts")
import devvnet_manager_aws as daws
import importlib

if len(sys.argv) < 2:
    print("Usage: shard-control.py (start|stop)")
    exit(-1)

command=sys.argv[1]

aws = daws.AWS(profile_name="shawn.mckenney", cluster='devv-shard1-cluster')

if command == 'stop':
    print("Stopping shard1")
    aws.stop_all()
elif command == 'start':
    print("Starting shard1")
    aws.start_all()
else:
    print("ERROR: I don't know what {} means.".format(command))

aws.summarize()
