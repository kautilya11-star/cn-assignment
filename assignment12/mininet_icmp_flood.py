#!/usr/bin/env python3

from mininet.net import Mininet
from mininet.node import Controller
from mininet.cli import CLI
from mininet.log import setLogLevel, info
from mininet.link import TCLink

def createNetwork():
    net = Mininet(controller=Controller, link=TCLink)
    
    info('*** Adding controller\n')
    net.addController('c0')
    
    info('*** Adding hosts\n')
    # Attacker (h1)
    attacker = net.addHost('h1', ip='10.0.0.1')
    
    # Victim (h2)
    victim = net.addHost('h2', ip='10.0.0.2')
    
    # 4 Agent hosts (spoofed IPs)
    agents = []
    for i in range(3, 7):  # h3 to h6
        agent = net.addHost('h%d' % i, ip='10.0.0.%d' % i)
        agents.append(agent)
    
    info('*** Adding switch\n')
    switch = net.addSwitch('s1')
    
    info('*** Creating links\n')
    net.addLink(attacker, switch)
    net.addLink(victim, switch)
    for agent in agents:
        net.addLink(agent, switch)
    
    info('*** Starting network\n')
    net.start()
    
    info('*** Setting up spoofed IPs on agents\n')
    # Configure agents with additional spoofed IPs for ICMP
    spoofed_ips = ['192.168.1.105', '192.168.1.106', '192.168.1.107', '192.168.1.108']
    for i, agent in enumerate(agents):
        agent.cmd('ifconfig h%d-eth0:1 %s netmask 255.255.255.0' % (i+3, spoofed_ips[i]))
    
    info('*** Testing connectivity\n')
    print(net.pingAll())
    
    info('*** Instructions for testing:\n')
    info('1. On h2 (victim), run: tcpdump -i any -w icmp_flood.pcap\n')
    info('2. On h1 (attacker), compile and run icmp_flood.c\n')
    info('3. Monitor traffic with wireshark\n')
    
    CLI(net)
    
    info('*** Stopping network\n')
    net.stop()

if __name__ == '__main__':
    setLogLevel('info')
    createNetwork()
