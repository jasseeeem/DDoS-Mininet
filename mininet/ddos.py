from mininet.topo import Topo
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.cli import CLI

class MyTopo( Topo ):
    # "2 switch 2 host custom topology"

    def __init__( self ):
        # "Create custom topo."

        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        h1 = self.addHost( 'h1' )
        h2 = self.addHost( 'h2' )
        s1 = self.addSwitch( 's1' )
        s2 = self.addSwitch( 's2' )

        # Add links
        self.addLink( h1, s1 )
        self.addLink( h2, s2 )
        self.addLink( s1, s2 )

    def addFlows(self, net):
        # Add flows
        s1 = net.get('s1')
        s2 = net.get('s2')

        # Add flows
        flow1 = {'nw_dst': '10.0.0.2', 'actions': 'output:2'}
        flow2 = {'nw_dst': '10.0.0.1', 'actions': 'output:1'}
        flow3 = {'nw_dst': '10.0.0.1', 'actions': 'output:2'}
        flow4 = {'nw_dst': '10.0.0.2', 'actions': 'output:1'}
        s1.cmd('ovs-ofctl add-flow -O OpenFlow13 %s "ip,%s"' % ('s1', ','.join(['%s=%s' % (k, v) for k, v in flow1.items()])))
        s1.cmd('ovs-ofctl add-flow -O OpenFlow13 %s "ip,%s"' % ('s1', ','.join(['%s=%s' % (k, v) for k, v in flow2.items()])))
        s2.cmd('ovs-ofctl add-flow -O OpenFlow13 %s "ip,%s"' % ('s2', ','.join(['%s=%s' % (k, v) for k, v in flow3.items()])))
        s2.cmd('ovs-ofctl add-flow -O OpenFlow13 %s "ip,%s"' % ('s2', ','.join(['%s=%s' % (k, v) for k, v in flow4.items()])))

TOPOS = {'MyTopo' : (lambda : MyTopo())}

def main():
    topo = MyTopo()
    net = Mininet(topo,link=TCLink)
    net.start()
    topo.addFlows(net)
    CLI(net)
    net.stop()

if __name__ == "__main__":
    main()