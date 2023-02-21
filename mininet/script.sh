# For starting the server on h9
#xterm h9
# python3 -m http.server 80

# For pinging from h1 to h2 and vice-versa
sh ovs-ofctl add-flow s1 priority=8000,ip,nw_dst=10.0.0.2,actions=output:2
sh ovs-ofctl add-flow s1 priority=8000,ip,nw_dst=10.0.0.1,actions=output:1

# For sending request to server in h9 from h5
sh ovs-ofctl add-flow s2 priority=8000,ip,nw_dst=10.0.0.9,actions=output:4
sh ovs-ofctl add-flow s2 priority=8000,ip,nw_dst=10.0.0.5,actions=output:1

# sh ovs-ofctl add-flow s2 priority=500,dl_type=0x800,nw_proto=6,tp_dst=80,actions=output:1
# sh ovs-ofctl add-flow s2 priority=800,ip,nw_src=10.0.0.5,actions=normal
# sh ovs-ofctl add-flow s2 arp,actions=normal

# For forwarding from s3 to h9
sh ovs-ofctl add-flow s3 priority=8000,ip,nw_dst=10.0.0.9,actions=output:1
sh ovs-ofctl add-flow s3 priority=8000,ip,nw_dst=10.0.0.5,actions=output:2
