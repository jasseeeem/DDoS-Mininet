sh ovs-ofctl add-flow s2 priority=800,ip,nw_src=10.0.0.5,actions=normal
sh ovs-ofctl add-flow s2 priority=500,dl_type=0x800,nw_proto=6,tp_dst=80,actions=output:1
sh ovs-ofctl add-flow s2 arp,actions=normal
xterm h5