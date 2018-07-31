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
        self._working_dir = devvnet['working_dir']

        current_port = self._base_port
        for i in self._devvnet['shards']:
            print("Adding shard {}".format(i['shard_index']))
            s = Shard(i)
            current_port = s.initialize_bind_ports(current_port)
            s.evaluate_hostname()
            s.connect_shard_nodes()
            self._shards.append(s)


        for i,shard in enumerate(self._shards):
            print("shard: "+ str(shard))
            for i2,node in enumerate(shard.get_nodes()):
                node.grill_raw_subs(shard.get_index())

                for rsub in node.get_raw_subs():
                    n = self.get_shard(rsub.get_shard_index()).get_node(rsub._name, rsub._node_index)
                    node.add_subscriber(n.get_host(), n.get_port())

                node.add_working_dir(self._working_dir)

    def __str__(self):
        s = "Devvnet\n"
        s += "base_port   : "+str(self._base_port)+"\n"
        s += "working_dir : "+str(self._working_dir)+"\n"
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


    def evaluate_hostname(self):
        for node in self._nodes:
            node.set_host(node.get_host().replace("${node_index}", str(node.get_index())))
            if node.get_host().find("format") > 0:
                print("formatting")
                node.set_host(eval(node.get_host()))

            node.evaluate_hostname()

    def get_nodes(self):
        return self._nodes

    def get_num_nodes(self):
        return len(self._nodes)

    def get_node(self, name, index):
        node = [x for x in self._nodes if (x.get_name() == name and x.get_index() == index)]
        if len(node) == 0:
            return None

        if len(node) != 1:
            raise("WOOP: identical nodes? ")

        return node[0]
        #node = [y for y in nodes if y.get_index() == index
        #shard1_validators = [x for x in conf['devvnet']['shards'][1]['process'] if x['name'] == 'validator']

    def get_index(self):
        return self._shard_index

class RawSub():
    def __init__(self, name, shard_index, node_index):
        self._name = name
        self._shard_index = shard_index
        self._node_index = node_index

    def __str__(self):
        sub = "({}:{}:{})".format(self._name, self._shard_index, self._node_index)
        return sub

    def get_shard_index(self):
        return self._shard_index

    def substitute_node_index(self, node_index):
        if self._node_index == "${node_index}":
            self._node_index = int(node_index)
        else:
            print("WARNING: not subbing "+str(self._node_index)+" with "+str(node_index))
        return
    '''
        try:
            print("n: "+str(self._node_index))
            n = self._node_index.replace('\${node_index}', node_index)
            print("n2: "+str(n))
            self._node_index = int(n)
            print("n: "+str(self._node_index))
        except:
            print("nothing to sub")
            raise
    '''

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

    def set_host(self, hostname):
        self._host = hostname

    def get_port(self):
        return self._port

    def set_port(self, port):
        self._port = port


class Node():
    def __init__(self, shard_index, index, name, host, port = 0):
        self._name = name
        self._shard_index = int(shard_index)
        self._index = int(index)
        self._host = host
        self._bind_port = int(port)
        self._subscriber_list = []
        self._raw_sub_list = []
        self._working_directory = ""

    def __str__(self):
        subs = "s["
        for sub in self._subscriber_list:
            subs += str(sub)
        subs += "]"
        rawsubs = "r["
        for rawsub in self._raw_sub_list:
            rawsubs += str(rawsub)
        rawsubs += "]"
        s = "node({}:{}:{}:{}:{}) {} {}".format(self._name, self._index, self._host, self._bind_port, self._working_directory, subs, rawsubs)
        return s

    def add_working_dir(self, directory):
        wd = directory.replace("${name}", self._name)
        wd = wd.replace("${shard_index}", str(self._shard_index))
        wd = wd.replace("${node_index}", str(self.get_index()))
        self._working_directory = wd

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

    def evaluate_hostname(self):
        for sub in self._subscriber_list:
            sub.set_host(sub.get_host().replace("${node_index}", str(self.get_index())))
            if sub.get_host().find("format") > 0:
                print("formatting")
                sub.set_host(eval(sub.get_host()))

    def grill_raw_subs(self, shard_index):
        for sub in self._raw_sub_list:
            sub.substitute_node_index(self._index)
            #d = subs.replace("${node_index}", str(self._index))
            print("up "+str(sub))

    def get_raw_subs(self):
        return self._raw_sub_list

    def get_name(self):
        return self._name


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
    shard_index = yml_dict['shard_index']
    for proc in yml_dict['process']:
        ind = proc['node_index']
        print(ind)

        try:
            if ind.find(',') > 0:
                ns = ind.split(',')
                print("creating {} {} processes".format(len(ns), proc['name']))
                for n in ns:
                    node = Node(shard_index, n, proc['name'], proc['host'], proc['bind_port'])
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
                nodes.append(Node(shard_index, ind, proc['name'], proc['host'], proc['bind_port']))
                print("creating a "+proc['name']+" process")
        except:
            nodes.append(Node(shard_index, ind, proc['name'], proc['host'], proc['bind_port']))
            print("creating a "+proc['name']+" process")
    return nodes
