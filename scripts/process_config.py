import yaml

# Read in config file
with open("../opt/devvnet_config.yaml", "r") as f:
    buf = ''.join(f.readlines())
    conf = yaml.load(buf, Loader=yaml.Loader)

# Set bind_port values
port = conf['devvnet']['base_port']
for a in conf['devvnet']['shards']:
    for b in a['process']:
        port = port + 1
        b['bind_port'] = port


shard1_validators = [x for x in conf['devvnet']['shards'][1]['process'] if x['name'] == 'validator']

# Add subscription endpoints
for v in shard1_validators:
    try:
        v['subscribe_endpoint']
    except:
        v['subscribe_endpoint'] = []

# Connect the shard nodes
for i in shard1_validators:
    cs = dict()
    cs['host'] = i['host']
    cs['bind_port'] = i['bind_port']
    for j in shard1_validators:
        if i['node_index'] == j['node_index']:
            continue
        i['subscribe_endpoint'].append(cs)


def get_devvnet(filename):
    with open(filename, "r") as f:
        buf = ''.join(f.readlines())
        conf = yaml.load(buf, Loader=yaml.Loader)

    # Set bind_port values
    port = conf['devvnet']['base_port']
    for a in conf['devvnet']['shards']:
        for b in a['process']:
            port = port + 1
            b['bind_port'] = port
    return(conf['devvnet'])


class Devvnet(object):
    _base_port = 0
    _password_file = ""
    _working_dir = ""

    def __init__(self, devvnet):
        self._devvnet = devvnet
        self._shards = []
        for i in self._devvnet['shards']:
            print("Adding shard {}".format(i['shard_index']))
            self._shards.append(Shard(i))


class Shard(object):
    _shard_index = 0;
    _password_file = ""
    _working_dir = ""
    _shard = None
    _nodes = []

    def __init__(self, shard):
        self._shard = shard
        self._nodes = get_nodes(shard)


    def _connect_shard_nodes(self):
        v_index = [i for i,x in enumerate(self._nodes) if x.is_validator()]

        for i in v_index:
            host = self._nodes[i].get_host()
            port = self._nodes[i].get_port()
            print("setting port to {}".format(port))
            for j in v_index:
                if i == j:
                    continue
                self._nodes[j].add_subscriber(host, port)


class Sub():
    def __init__(self, host, port):
        self._host = host
        self._port = port

    def __str__(self):
        sub = "({}:{})".format(self.get_host(), str(self.get_port()))
        return sub

    def __eq__(self, other):
        if self._host != other.get_host():
            return False
        if self._port != other.get_port():
            return False
        return True

    def get_host(self):
        return self._host

    def get_port(self):
        return self._port


class Node():
    def __init__(self, index, name, host, port):
        self._name = name
        self._index = index
        self._host = host
        self._bind_port = port
        self._subscriber_list = []

    def __str__(self):
        subs = "["
        for sub in self._subscriber_list:
            subs += str(sub)
        subs += "]"
        s = "node({}:{}:{}:{}) {}".format(self._name, self._index, self._host, self._port, subs)
        return s

    def is_validator(self):
        return(self._name == "validator")

    def add_subscriber(self, host, port):
        self._subscriber_list.append(Sub(host,port))

    def get_host(self):
        return self._host

    def get_port(self):
        return self._bind_port


def get_nodes(yml_dict):
    nodes = []
    for proc in yml_dict['process']:
        ind = proc['node_index']
        print(ind)

        try:
            if ind.find(',') > 0:
                ns = ind.split(',')
                print("creating {} {} processes".format(len(ns), proc['name']))
                for n in ns:
                    nodes.append(Node(n, proc['name'], proc['host'], proc['bind_port']))
            #elif ind.find('..') > 0:
            #    n = range(
            else:
                nodes.append(Node(ind, proc['name'], proc['host'], proc['bind_port']))
                print("creating a "+proc['name']+" process")
        except:
            nodes.append(Node(ind, proc['name'], proc['host'], proc['bind_port']))
            print("creating a "+proc['name']+" process")
    return nodes
