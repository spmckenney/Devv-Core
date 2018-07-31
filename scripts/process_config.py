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

        '''
        for i,s in enumerate(self._devvnet['shards']):
            p = s['process']
            for i2,n in enumerate(p['subscribe']):
                print("Yay!!")
        '''


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
        a_index = [i for i,x in enumerate(self._nodes) if x.is_announcer()]
        r_index = [i for i,x in enumerate(self._nodes) if x.is_repeater()]

        print(v_index)
        print(a_index)

        for i in v_index:
            host = self._nodes[i].get_host()
            port = self._nodes[i].get_port()
            print("setting port to {}".format(port))
            for j in v_index:
                if i == j:
                    continue
                self._nodes[j].add_subscriber(host, port)

            for k in a_index:
                announcer = self._nodes[k]
                if self._nodes[i].get_index() == announcer.get_index():
                    self._nodes[i].add_subscriber(announcer.get_host(), announcer.get_port())
                    break

            for l in r_index:
                print(type(self._nodes[i].get_index() ))
                if self._nodes[i].get_index() == self._nodes[l].get_index():
                    self._nodes[l].add_subscriber(host, port)

    def connect_intershard_comms(self):
        self._shar
        for node in self._nodes():
            pass

    def get_nodes(self):
        return self._nodes

    def get_num_nodes(self):
        return len(self._nodes)

class RawSub():
    def __init__(self, name, shard_index, node_index):
        self._name = name
        self._shard_index = shard_index
        self._node_index = node_index

    def __str__(self):
        sub = "({}:{}:{})".format(self._name, self._shard_index, self._node_index)
        return sub

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
        self._index = int(index)
        self._host = host
        self._bind_port = int(port)
        self._subscriber_list = []
        self._raw_sub_list = []

    def __str__(self):
        subs = "["
        for sub in self._subscriber_list:
            subs += str(sub)
        subs += "]"
        rawsubs = "["
        for rawsub in self._raw_sub_list:
            rawsubs += str(rawsub)
        rawsubs += "]"
        s = "node({}:{}:{}:{}) {} {}".format(self._name, self._index, self._host, self._bind_port, subs, raw_subs)
        return s

    def is_validator(self):
        return(self._name == "validator")

    def is_announcer(self):
        return(self._name == "announcer")

    def is_repeater(self):
        return(self._name == "repeater")

    def add_subscriber(self, host, port):
        self._subscriber_list.append(Sub(host,port))

    def add_raw_sub(self, name, shard_index, node_index):
        rs = RawSub(name,shard_index, node_index)
        print("adding rawsub: "+str(rs))
        self._raw_sub_list.append(rs)

    def get_index(self):
        return self._index

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
                    node = Node(n, proc['name'], proc['host'], proc['bind_port'])
                    try:
                        rawsubs = proc['subscribe']
                        for sub in proc['subscribe']:
                            try:
                                si = sub['shard_index']
                            except:
                                si = yml_dict['shard_index']
                            node.add_raw_sub(sub['name'], si, sub['node_index'])
                    except:
                        pass
                    nodes.append(node)
            #elif ind.find('..') > 0:
            #    n = range(
            else:
                nodes.append(Node(ind, proc['name'], proc['host'], proc['bind_port']))
                print("creating a "+proc['name']+" process")
        except:
            nodes.append(Node(ind, proc['name'], proc['host'], proc['bind_port']))
            print("creating a "+proc['name']+" process")
    return nodes
