#!/usr/bin/env python3

from mininet.net import Mininet
from mininet.node import Controller, OVSSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel, info
from mininet.link import TCLink

class LeafSpineTopo:
    def __init__(self, spine_count=2, leaf_count=4, hosts_per_leaf=2):
        self.spine_count = spine_count
        self.leaf_count = leaf_count
        self.hosts_per_leaf = hosts_per_leaf
        
    def build(self, net):
        info('*** Creating Spine Switches\n')
        spine_switches = []
        for i in range(1, self.spine_count + 1):
            spine = net.addSwitch('s%d' % i)
            spine_switches.append(spine)
            info('Added spine switch: %s\n' % spine.name)
        
        info('\n*** Creating Leaf Switches\n')
        leaf_switches = []
        for i in range(1, self.leaf_count + 1):
            leaf = net.addSwitch('l%d' % i)
            leaf_switches.append(leaf)
            info('Added leaf switch: %s\n' % leaf.name)
        
        info('\n*** Connecting Spine and Leaf Switches\n')
        # Full mesh: Each spine connects to every leaf
        for spine in spine_switches:
            for leaf in leaf_switches:
                net.addLink(spine, leaf)
                info('Link: %s <-> %s\n' % (spine.name, leaf.name))
        
        info('\n*** Creating Hosts\n')
        host_count = 1
        for i, leaf in enumerate(leaf_switches):
            for j in range(1, self.hosts_per_leaf + 1):
                host = net.addHost('h%d' % host_count, 
                                  ip='10.0.%d.%d/24' % (i+1, j),
                                  defaultRoute='via 10.0.%d.254' % (i+1))
                net.addLink(host, leaf)
                info('Host: %s (IP: 10.0.%d.%d) connected to %s\n' % 
                     (host.name, i+1, j, leaf.name))
                host_count += 1

def createLeafSpineNetwork():
    # Configuration
    SPINE_COUNT = 2
    LEAF_COUNT = 4
    HOSTS_PER_LEAF = 2
    
    info('\n========================================\n')
    info('LEAF-SPINE TOPOLOGY\n')
    info('========================================\n')
    info('Spine Switches: %d\n' % SPINE_COUNT)
    info('Leaf Switches: %d\n' % LEAF_COUNT)
    info('Hosts per Leaf: %d\n' % HOSTS_PER_LEAF)
    info('Total Hosts: %d\n' % (LEAF_COUNT * HOSTS_PER_LEAF))
    info('========================================\n\n')
    
    # Create network with proper controller
    net = Mininet(controller=Controller, switch=OVSSwitch, link=TCLink)
    
    info('*** Adding controller\n')
    net.addController('c0')
    
    # Create topology
    topo = LeafSpineTopo(SPINE_COUNT, LEAF_COUNT, HOSTS_PER_LEAF)
    topo.build(net)
    
    info('\n*** Starting network\n')
    net.start()
    
    # Add some delay for controller to set up
    import time
    time.sleep(3)
    
    info('\n*** Testing connectivity (pinging 3 times with 0.5s interval)\n')
    # Test ping between a few hosts instead of pingall
    hosts = net.hosts
    
    # Test 1: Hosts on same leaf
    info('Testing hosts on same leaf (h1 -> h2): ')
    result = net.ping([hosts[0], hosts[1]], timeout=1)
    info('Success\n' if result == 0 else 'Failed\n')
    
    # Test 2: Hosts on different leaves
    info('Testing hosts on different leaves (h1 -> h3): ')
    result = net.ping([hosts[0], hosts[2]], timeout=1)
    info('Success\n' if result == 0 else 'Failed\n')
    
    # Test 3: Hosts on far leaves
    info('Testing far hosts (h1 -> h7): ')
    result = net.ping([hosts[0], hosts[6]], timeout=1)
    info('Success\n' if result == 0 else 'Failed\n')
    
    info('\n*** Topology created successfully!\n')
    info('*** Type "pingall" to test all connections\n')
    info('*** Type "nodes" to see all nodes\n')
    info('*** Type "links" to see all links\n')
    info('*** Type "exit" to quit\n')
    
    CLI(net)
    
    info('\n*** Stopping network\n')
    net.stop()

def testScalability():
    info('\n========================================\n')
    info('SCALABILITY TEST - LEAF-SPINE TOPOLOGY\n')
    info('========================================\n')
    
    test_cases = [
        (2, 4, 2),   # Small
        (3, 6, 3),   # Medium  
        (4, 8, 4),   # Large
    ]
    
    for spine, leaf, hosts_per in test_cases:
        total_hosts = leaf * hosts_per
        total_links = spine * leaf + leaf * hosts_per
        
        info('\nConfiguration:\n')
        info('  Spine switches: %d\n' % spine)
        info('  Leaf switches: %d\n' % leaf)
        info('  Hosts per leaf: %d\n' % hosts_per)
        info('  Total hosts: %d\n' % total_hosts)
        info('  Total links: %d\n' % total_links)
        info('  Bisection bandwidth: %d links\n' % (spine * hosts_per))
        info('  ---\n')

if __name__ == '__main__':
    setLogLevel('info')
    
    # Show scalability information
    testScalability()
    
    # Create and run the network
    createLeafSpineNetwork()