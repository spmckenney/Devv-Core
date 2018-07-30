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
        self._base_port = devvnet['base_port']
        self._shards = []

        current_port = self._base_port
        for i in self._devvnet['shards']:
            print("Adding shard {}".format(i['shard_index']))
            s = Shard(i)
            current_port = s.initialize_bind_ports(current_port)
            s.connect_shard_nodes()
            self._shards.append(s)

    def __str__(self):
        s = "Devvnet\n"
        for shard in self._shards:
            s += str(shard)
        return s

    def get_shard(self, index):
        return self._shards[index]

    def get_num_nodes(self):
        count = 0
        for shard in self._shards:
            count += shard.get_num_nodes()
        return count


class Shard(object):
    _shard_index = 0;
    _password_file = ""
    _working_dir = ""
    _shard = None
    _nodes = []

    def __init__(self, shard):
        self._shard = shard
        self._nodes = get_nodes(shard)
        self._shard_index = self._shard['shard_index']

        try:
            self._name = self._shard['t1']
            self._type = "Tier1Shard"
        except:
            try:
                self._name = self._shard['t2']
                self._type = 'Tier2Shard'
            except:
                print("Error: Shard type neither Tier1 (t1) or Tier2 (t2)")

        #self._connect_shard_nodes()

    def __str__(self):
        s = "type: " + self._type + "\n"
        s += "index: " + str(self._shard_index) + "\n"
        for node in self._nodes:
            s += "  " + str(node) + "\n"
        return s

    def initialize_bind_ports(self, port_num):
        current_port = port_num
        for node in self._nodes:
            node.set_port(current_port)
            current_port = current_port + 1
        return current_port

    def connect_shard_nodes(self):
        v_index = [i for i,x in enumerate(self._nodes) if x.is_validator()]

        print(v_index)

        for i in v_index:
            host = self._nodes[i].get_host()
            port = self._nodes[i].get_port()
            print("setting port to {}".format(port))
            for j in v_index:
                if i == j:
                    continue
                self._nodes[j].add_subscriber(host, port)

    def get_nodes(self):
        return self._nodes

    def get_num_nodes(self):
        return len(self._nodes)


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
    def __init__(self, index, name, host, port = 0):
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
        s = "node({}:{}:{}:{}) {}".format(self._name, self._index, self._host, self._bind_port, subs)
        return s

    def is_validator(self):
        return(self._name == "validator")

    def add_subscriber(self, host, port):
        self._subscriber_list.append(Sub(host,port))

    def get_host(self):
        return self._host

    def set_host(self, host):
        self._host = host

    def get_port(self):
        return self._bind_port

    def set_port(self, port):
        self._bind_port = port


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
