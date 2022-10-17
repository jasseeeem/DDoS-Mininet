from mininet.net import Mininet
from mininet.log import lg
from mininet.cli import CLI
from mininet.node import CPULimitedHost

def main():
    lg.setLogLevel('info')

    # Construct the network with cpu limited hosts 
    net = Mininet(host = CPULimitedHost)

    net.addController('c0', ip="127.0.0.1", port=6653)
    
    # Create the network hosts, each having 5% of the system’s CPU
    h1 = net.addHost('h1', ip="10.0.0.1", cpu = 0.05)
    h2 = net.addHost('h2', ip="10.0.0.2", cpu=0.05)
    # h3 = net.addHost('h3', ip="10.0.0.3", cpu=0.05)
    # h4 = net.addHost('h4', ip="10.0.0.4", cpu=0.05)
    h5 = net.addHost('h5', ip="10.0.0.5", cpu=0.05)
    h6 = net.addHost('h6', ip="10.0.0.6", cpu=0.05)
    # h7 = net.addHost('h7', ip="10.0.0.7", cpu=0.05)
    # h8 = net.addHost('h8', ip="10.0.0.8", cpu=0.05)
    h9 = net.addHost('h9', ip="10.0.0.9", cpu=0.05)

    s1 = net.addSwitch('s1')
    s2 = net.addSwitch('s2')
    s3 = net.addSwitch('s3')

    net.addLink(s1, h1)
    net.addLink(s1, h2)
    # net.addLink(s1, h3)
    # net.addLink(s1, h4)

    net.addLink(s2, h5)
    net.addLink(s2, h6)
    # net.addLink(s2, h7)
    # net.addLink(s2, h8)

    net.addLink(s3, h9)

    net.addLink(s1, s2)
    net.addLink(s2, s3)

    # Mininet does not support ethernet loops (https://github.com/mininet/mininet/wiki/FAQ#ethernet-loops)
    # net.addLink(s3, s1)

    net.start()

    # h5 = net.get('h5')

    # h9 = net.get('h9')
    # p1 = h9.popen('python3 myServer.py -i %s &' % h9.IP())

    # h2 = net.get('h2')
    # h2.cmd('python3 myClient.py -i %s -m "hello world"' % h1.IP())
    
    CLI(net, script="script.sh")
    CLI(net)

    # p1.terminate()
    net.stop()

if __name__ == '__main__':
    main()
