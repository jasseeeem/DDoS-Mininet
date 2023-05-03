from mininet.topo import Topo
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.cli import CLI

NUM_PACKETS = 100000         # 100 pkts/sec 
NUM_FLOWS = 10
DURATION = 100000       # 100 seconds

class MyTopo( Topo ):
    # "2 switch 2 host custom topology"

    def __init__( self ):
        # "Create custom topo."

        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        h2 = self.addHost('h2', ip='10.0.0.1/24', ipv6='2001:db8::1/64')
        h1 = self.addHost('h1', ip='10.0.0.2/24', ipv6='2001:db8::2/64')
        h3 = self.addHost('h3', ip='10.0.0.3/24', ipv6='2001:db8::3/64')
        h4 = self.addHost('h4', ip='10.0.0.4/24', ipv6='2001:db8::4/64')
        h5 = self.addHost('h5', ip='10.0.0.5/24', ipv6='2001:db8::5/64')
        h6 = self.addHost('h6', ip='10.0.0.6/24', ipv6='2001:db8::6/64')
        h7 = self.addHost('h7', ip='10.0.0.7/24', ipv6='2001:db8::7/64')
        h8 = self.addHost('h8', ip='10.0.0.8/24', ipv6='2001:db8::8/64')
        h9 = self.addHost('h9', ip='10.0.0.9/24', ipv6='2001:db8::9/64')
        s1 = self.addSwitch('s1')
        s2 = self.addSwitch('s2')
        s3 = self.addSwitch('s3')
        s4 = self.addSwitch('s4')

        # Add links
        self.addLink(h1, s1)
        self.addLink(h2, s2)
        self.addLink(h3, s2)
        self.addLink(h4, s2)
        self.addLink(h5, s3)
        self.addLink(h6, s3)
        self.addLink(h7, s3)
        self.addLink(h8, s4)
        self.addLink(h9, s4)
        self.addLink(s1, s2)
        self.addLink(s2, s3)
        self.addLink(s2, s4)
        

TOPOS = {'MyTopo' : (lambda : MyTopo())}

def main():
    topo = MyTopo()
    net = Mininet(topo, link=TCLink)
    net.start()

    # Get a handle to h1
    h1 = net.get('h1')
    h2 = net.get('h2')
    h3 = net.get('h3')
    h4 = net.get('h4')
    h5 = net.get('h5')
    h6 = net.get('h6')
    h7 = net.get('h7')
    h8 = net.get('h8')
    h9 = net.get('h9')

    # Run HCF module
    # h2.cmd(f"rm -rf /home/jaseem/Documents/DDoS-Mininet/hcf/hdf5.h5 && sudo /home/jaseem/Documents/DDoS-Mininet/hcf/a.out &")
    # h3.cmd(f"rm -rf /home/jaseem/Documents/DDoS-Mininet/hcf/hdf5.h5 && sudo /home/jaseem/Documents/DDoS-Mininet/hcf/a.out &")
    # h1.cmd(f"rm -rf /home/jaseem/Documents/DDoS-Mininet/hcf/hdf5.h5 && sudo /home/jaseem/Documents/DDoS-Mininet/hcf/a.out &")
    # h5.cmd(f"rm -rf /home/jaseem/Documents/DDoS-Mininet/hcf/hdf5.h5 && sudo /home/jaseem/Documents/DDoS-Mininet/hcf/a.out &")
    # h7.cmd(f"rm -rf /home/jaseem/Documents/DDoS-Mininet/hcf/hdf5.h5 && sudo /home/jaseem/Documents/DDoS-Mininet/hcf/a.out &")
    # h9.cmd(f"rm -rf /home/jaseem/Documents/DDoS-Mininet/hcf/hdf5.h5 && sudo /home/jaseem/Documents/DDoS-Mininet/hcf/a.out &")
    
    # Run the servers
    h4.cmd("/home/jaseem/Downloads/D-ITG-2.8.1-r1023/bin/ITGRecv &")
    h6.cmd("/home/jaseem/Downloads/D-ITG-2.8.1-r1023/bin/ITGRecv &")
    h8.cmd("/home/jaseem/Downloads/D-ITG-2.8.1-r1023/bin/ITGRecv &")

    # Run HTTP simulation
    h1.cmd(f"/home/jaseem/Downloads/D-ITG-2.8.1-r1023/bin/ITGSend -d 10000 -T TCP -a 10.0.0.6 -z {NUM_PACKETS} -C {NUM_FLOWS} -t {DURATION} &")
    h2.cmd(f"/home/jaseem/Downloads/D-ITG-2.8.1-r1023/bin/ITGSend -d 10000 -T TCP -a 10.0.0.8 -z {NUM_PACKETS} -C {NUM_FLOWS} -t {DURATION} &")
    h3.cmd(f"/home/jaseem/Downloads/D-ITG-2.8.1-r1023/bin/ITGSend -d 10000 -T TCP -a 10.0.0.4 -z {NUM_PACKETS} -C {NUM_FLOWS} -t {DURATION} &")
    h5.cmd(f"/home/jaseem/Downloads/D-ITG-2.8.1-r1023/bin/ITGSend -d 10000 -T TCP -a 10.0.0.4 -z {NUM_PACKETS} -C {NUM_FLOWS} -t {DURATION} &")
    h7.cmd(f"/home/jaseem/Downloads/D-ITG-2.8.1-r1023/bin/ITGSend -d 10000 -T TCP -a 10.0.0.8 -z {NUM_PACKETS} -C {NUM_FLOWS} -t {DURATION} &")
    h9.cmd(f"/home/jaseem/Downloads/D-ITG-2.8.1-r1023/bin/ITGSend -d 10000 -T TCP -a 10.0.0.6 -z {NUM_PACKETS} -C {NUM_FLOWS} -t {DURATION} &")
    
    # print("RESULT: ", result)
    CLI(net)
    
    net.stop()

if __name__ == "__main__":
    main()