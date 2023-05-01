from math import log2
from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import MAIN_DISPATCHER, CONFIG_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.ofproto import ofproto_v1_3
from ryu.app.wsgi import ControllerBase, WSGIApplication, route, Response
import json
import requests
from ryu.lib.packet import ether_types
from scapy.all import *
from scapy.layers.l2 import Ether
import ryu.lib.hub as hub
from ryu.topology import api as topo_api
import numpy as np

MAX_DATAPOINTS = 5
FLOW_SD_STEPS = 1
PACKET_RATE_SD_STEPS = 1
ENTROPY_STEPS = 1


class PacketRateMonitor(app_manager.RyuApp):
    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]
    _CONTEXTS = {'wsgi': WSGIApplication}

    def __init__(self, *args, **kwargs):
        super(PacketRateMonitor, self).__init__(*args, **kwargs)
        wsgi = kwargs['wsgi']
        wsgi.register(PacketRateMonitorController)
        self.mac_to_port = {}
        self.packet_count = {}
        self.flow_count = {}    #Will this be blank every time we request for a switch?

        # Start the periodic stats request timer
        self.monitor_thread = hub.spawn(self._monitor)

    def _monitor(self):
        while True:
            switches = topo_api.get_switch(self, None)
            for switch in switches:
                self._request_port_stats(switch.dp)
                self._calc_sd_flow_count(switch.dp)
                self._calc_sd_packet_rate(switch.dp)
                self._calc_sd_entropy(switch.dp)
            
            self.send_entropy_data(self.packet_count)
            hub.sleep(5) # Request port stats every 5 seconds

    def _calc_sd_packet_rate(self, datapath):
        url = 'http://127.0.0.1:8080/packet_rate'
        response = requests.get(url)
        print(f"Packet Rate Outputs")
        if response.status_code == 200:
            packet_rate_data = response.json()
        else:
            print(f"Error: {response.status_code} - {response.text}")
        
        for switch_id, switch_data in packet_rate_data.items():
            print(f"Switch {switch_id}:")
            for port_no, port_data in switch_data.items():
                #packet_rates = port_data['packet_rate']
                sd=np.std(port_data)
                mean=sum(port_data)/len(port_data)
                if( abs( ((port_data[-1]-mean)/sd) > PACKET_RATE_SD_STEPS)):
                    print(f"❌ Switch {switch_id} Port {port_no} is showing anomaly")
                else:
                    print(f"✅ Switch {switch_id} Port {port_no} is fine")
                print(f"  Port {port_no}: {port_data}, {port_data[:-1]}, {port_data[-1]} Avg: {mean}, SD: {sd}")
        return
    
    def _calc_sd_flow_count(self, datapath):
        url = 'http://127.0.0.1:8080/flow_count'
        response = requests.get(url)
        print(f"Flow Count Outputs:")
        if response.status_code == 200:
            flow_count_data = response.json()
            print(flow_count_data)  # print the packet rate data
        else:
            print(f"Error: {response.status_code} - {response.text}")
        
        for switch_id, switch_data in flow_count_data.items():
            print(f"Switch {switch_id}:")
            for port_no, port_data in switch_data.items():
                #packet_rates = port_data['packet_rate']
                sd=np.std(port_data)
                mean=sum(port_data)/len(port_data)
                if( abs( ((port_data[-1]-mean)/sd) > FLOW_SD_STEPS)):
                    print(f"❌ Switch {switch_id} Port {port_no} is showing anomaly")
                else:
                    print(f"✅ Switch {switch_id} Port {port_no} is fine")
                print(f"  Port {port_no}: {port_data}, {port_data[:-1]}, {port_data[-1]} Avg: {mean}, SD: {sd}")
        return
    
    def _calc_sd_entropy(self, datapath):
        url = 'http://127.0.0.1:8080/entropy'
        response = requests.get(url)
        print(f"Entropy Outputs:")
        if response.status_code == 200:
            entropy_data = response.json()
            print(entropy_data)  # print the entropy data
        else:
            print(f"Error: {response.status_code} - {response.text}")
        
        for switch_id, switch_data in entropy_data.items():
            print(f"Switch {switch_id}:")
            for port_no, port_data in switch_data.items():
                sd = np.std(port_data)
                mean = sum(port_data)/len(port_data)
                if( abs( ((port_data[-1]-mean)/sd) > ENTROPY_STEPS)):
                    print(f"❌ Switch {switch_id} Port {port_no} is showing anomaly")
                else:
                    print(f"✅ Switch {switch_id} Port {port_no} is fine")
                print(f"  Port {port_no}: {port_data}, {port_data[:-1]}, {port_data[-1]} Avg: {mean}, SD: {sd}")
        return


    def _request_port_stats(self, datapath):
        parser = datapath.ofproto_parser
        ofproto = datapath.ofproto
        print(f"🟡\tRequesting Stats from Switch {datapath.id}")

        # Send flow stats request for the switch
        req = parser.OFPFlowStatsRequest(datapath)
        datapath.send_msg(req)
    
        # send port stats request for the switch
        for port in datapath.ports.values():
            if port.port_no != 4294967293:
                print(f"\tRequesting from port {port.port_no}")
                req = parser.OFPPortStatsRequest(datapath, 0, port.port_no)
                datapath.send_msg(req)

    def add_flow(self, datapath, priority, match, actions, buffer_id=None):
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS, actions)]
        if buffer_id:
            mod = parser.OFPFlowMod(datapath=datapath, buffer_id=buffer_id,
                                    priority=priority, match=match,
                                    instructions=inst)
        else:
            mod = parser.OFPFlowMod(datapath=datapath, priority=priority,
                                    match=match, instructions=inst)
        datapath.send_msg(mod)

    @set_ev_cls(ofp_event.EventOFPPacketIn, MAIN_DISPATCHER)
    def _packet_in_handler(self, ev):        
        # If you hit this you might want to increase
        # the "miss_send_length" of your switch
        if ev.msg.msg_len < ev.msg.total_len:
            self.logger.debug("packet truncated: only %s of %s bytes",
                              ev.msg.msg_len, ev.msg.total_len)
        msg = ev.msg
        datapath = msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        in_port = msg.match['in_port']

        eth = Ether(msg.data)
        
        if eth.type == ether_types.ETH_TYPE_LLDP:
            # ignore LLDP packet
            return

        dst_mac = eth.dst
        src_mac = eth.src

        dpid = format(datapath.id, "d").zfill(16)
        self.mac_to_port.setdefault(dpid, {})

        # used to calculate the entropy
        key = '-'.join([str(dpid), str(in_port), src_mac, dst_mac])
        self.packet_count[key] = self.packet_count.get(key, 0) + 1

        # self.logger.info("packet in %s %s %s %s", dpid, src, dst, in_port)
        # self.logger.info("packet in %s %s %s %s", dpid, src_mac, dst_mac, in_port)

        # learn a mac address to avoid FLOOD next time.
        self.mac_to_port[dpid][src_mac] = in_port

        if dst_mac in self.mac_to_port[dpid]:
            out_port = self.mac_to_port[dpid][dst_mac]
        else:
            out_port = ofproto.OFPP_FLOOD

        actions = [parser.OFPActionOutput(out_port)]

        # install a flow to avoid packet_in next time
        if out_port != ofproto.OFPP_FLOOD:
            match = parser.OFPMatch(in_port=in_port, eth_dst=dst_mac, eth_src=src_mac)
            # verify if we have a valid buffer_id, if yes avoid to send both
            # flow_mod & packet_out
            if msg.buffer_id != ofproto.OFP_NO_BUFFER:
                self.add_flow(datapath, 1, match, actions, msg.buffer_id)
                return
            else:
                self.add_flow(datapath, 1, match, actions)
        data = None
        if msg.buffer_id == ofproto.OFP_NO_BUFFER:
            data = msg.data

        out = parser.OFPPacketOut(datapath=datapath, buffer_id=msg.buffer_id,
                                  in_port=in_port, actions=actions, data=data)
        datapath.send_msg(out)

    # triggered when switch sends EventOFPSwitchFeatures message to controller - indicates that switch has connected to controller and is ready to communicate
    @set_ev_cls(ofp_event.EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        datapath = ev.msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        # match all packets
        match = parser.OFPMatch()
        #match.set_in_port(ofproto.OFPP_ANY)
        # send them to the controller
        actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER,
                                          ofproto.OFPCML_NO_BUFFER)]
        self.add_flow(datapath, 0, match, actions)

        # Enable port statistics for the switch
        req = parser.OFPPortStatsRequest(datapath, 0, ofproto.OFPP_ANY)
        datapath.send_msg(req)

        req = parser.OFPFlowStatsRequest(datapath, 0, ofproto.OFPTT_ALL, ofproto.OFPP_ANY, ofproto.OFPMPF_REQ_MORE)
        datapath.send_msg(req)

        # Enable flow statistics for the switch
        #req = parser.OFPFlowStatsRequest(datapath, 0, ofproto.OFPF_ANY)
        req = parser.OFPFlowStatsRequest(datapath, 0, ofproto.OFPTT_ALL,
                                 ofproto.OFPP_ANY, ofproto.OFPG_ANY,
                                 0, 0, parser.OFPMatch())

        datapath.send_msg(req)


        # Create an OFPMatch object with no match criteria
        match = parser.OFPMatch()

        # Enable aggregate statistics to retrieve statistics about the entire switch, such as the number of packets and bytes processed by the switch
        req = parser.OFPAggregateStatsRequest(datapath, 0, cookie=0, cookie_mask=0, match=match, table_id=ofproto.OFPTT_ALL, out_port=ofproto.OFPP_ANY, out_group=ofproto.OFPG_ANY)
        
        #self.switches[datapath.id] = datapath

        datapath.send_msg(req)

    @set_ev_cls(ofp_event.EventOFPPortStatsReply, MAIN_DISPATCHER)
    def port_stats_reply_handler(self, ev):
        datapath = ev.msg.datapath
        # get the body of the port statistics reply message
        stats = ev.msg.body

        # loop over each port in the statistics
        for stat in stats:
            port_no = stat.port_no
            # number of received packets
            rx_packets = stat.rx_packets
            # number of transmitted packets - not considering the packets leaving the switch
            # tx_packets = stat.tx_packets
            duration_sec = stat.duration_sec
            duration_nsec = stat.duration_nsec
            
            # Store packet rate data in a dictionary
            packet_rate_data = { 'switch_id': datapath.id, 'port_no': port_no, 'num_packets': rx_packets, 'time': duration_sec + duration_nsec / 1e9}
            self.send_packet_rate_data(packet_rate_data)

        #self.send_entropy_data(self.packet_count)

    def send_packet_rate_data(self, data):
        # Send the packet rate data to the Ryu REST API server
        url = 'http://127.0.0.1:8080/packet_rate'
        response = requests.post(url, json=data)
        if response.status_code != 200:
            self.logger.error('Failed to send packet rate data')

    # Send the entropy data to the RYU REST API server
    def send_entropy_data(self, data):
        url = 'http://127.0.0.1:8080/entropy'
        response = requests.post(url, json=data)
        if response.status_code != 200:
            self.logger.error('Failed to send entropy data')

    @set_ev_cls(ofp_event.EventOFPFlowStatsReply, MAIN_DISPATCHER)
    def flow_stats_reply_handler(self, ev):
        datapath = ev.msg.datapath
        # get the body of the port statistics reply message
        stats = ev.msg.body

        if len(stats) > 0:
            # loop over each port in the statistics
            flow_data = {}
            for stat in stats:
                # check if there is a field called in_port
                try:
                    port_no = int(stat.match['in_port'])
                except KeyError:
                    port_no = None
                if port_no:
                    if port_no in flow_data.keys():
                        flow_data[port_no] += 1
                    else:
                        flow_data[port_no] = 1

            if len(flow_data) != 0:
                self.send_flow_count_data({'switch_id': datapath.id, 'count': flow_data})

    def send_flow_count_data(self, data):
        url = 'http://127.0.0.1:8080/flow_count'
        response = requests.post(url, json=data)
        if response.status_code != 200:
            self.logger.error('Failed to send flow count data')


# REST API controller class
class PacketRateMonitorController(ControllerBase):
    # store as class attribute so that it is not reset after every API call (which is what happens if put inside __init__)
    packet_rate_data = {}
    entropy_data = {}
    flow_count_data = {}

    # When a new HTTP request is received, the web framework instantiates a new instance of the controller class to handle the request. 
    # The __init__ method is called during the instantiation process to initialize the attributes of the new instance. 
    # The attributes of the previous instances are not preserved since they are specific to that instance and will be destroyed when the instance is no longer needed.
    def __init__(self, req, link, data, **config):
        super(PacketRateMonitorController, self).__init__(req, link, data, **config)
        self.packet_rate_data = {} 

    # update the packet rate of a switch
    @route('packet_rate_monitor', '/packet_rate', methods=['POST'])
    def packet_rate(self, req, **kwargs):
        data = req.json
        if data['switch_id'] in PacketRateMonitorController.packet_rate_data.keys():
            packet_diff = 0
            time_diff = 0
            if data['port_no'] in PacketRateMonitorController.packet_rate_data[data['switch_id']].keys() :
                packet_diff = data['num_packets'] - PacketRateMonitorController.packet_rate_data[data['switch_id']][data['port_no']]['prev_num_packets']
                time_diff = data['time'] - PacketRateMonitorController.packet_rate_data[data['switch_id']][data['port_no']]['prev_time']                                                                                                           
            else:
                packet_diff = data['num_packets']
                time_diff = data['time']
            prev_packet_rate = []
            if data['port_no'] in PacketRateMonitorController.packet_rate_data[data['switch_id']].keys():
                prev_packet_rate = PacketRateMonitorController.packet_rate_data[data['switch_id']][data['port_no']]['packet_rate']
            PacketRateMonitorController.packet_rate_data[data['switch_id']][data['port_no']] = {'packet_rate': prev_packet_rate[-MAX_DATAPOINTS + 1 :] + [packet_diff / time_diff], 'prev_num_packets': data['num_packets'], 'prev_time': data['time']}
        else:
            PacketRateMonitorController.packet_rate_data[data['switch_id']] = {data['port_no']: {'packet_rate': [data['num_packets'] / data['time']], 'prev_num_packets': data['num_packets'], 'prev_time': data['time']}}
        return Response(status=200)

    # get the packet rates of all switches
    @route('packet_rate_monitor', '/packet_rate', methods=['GET'])
    def get_packet_rate(self, req, **kwargs):
        packet_rates = {}
        for switch_id, switch_data in PacketRateMonitorController.packet_rate_data.items():
            packet_rates[switch_id] = {}
            for port_no, port_data in switch_data.items():
                packet_rates[switch_id][port_no] = port_data['packet_rate']
        return json.dumps(packet_rates) 

    # update the entropy of a switch
    @route('entropy_monitor', '/entropy', methods=['POST'])
    def post_entropy(self, req, **kwargs):
        #PacketRateMonitorController.entropy_data = {} #to remove
        data = req.json
        total_packets = sum(data.values())
        interface_entropy = {}
        print(f"entropy:")
        print(json.dumps(PacketRateMonitorController.entropy_data))

        for key in data:
            [datapath_id, in_port, src, dst] = key.split("-")
            probability = data[key] / total_packets
            interface_entropy['-'.join([datapath_id, in_port])] = interface_entropy.get('-'.join([datapath_id, in_port]), 0) - probability * log2(probability)
            #interface_entropy[datapath_id] = {in_port: interface_entropy[datapath_id].get(in_port, 0) - probability * log2(probability)}
        for key in interface_entropy:
            #print(key)
            
            [datapath_id, in_port] = key.split("-")
            if datapath_id in PacketRateMonitorController.entropy_data:
                if in_port in PacketRateMonitorController.entropy_data[datapath_id]:
                    PacketRateMonitorController.entropy_data[datapath_id][in_port].append(interface_entropy[key])
                else:
                    PacketRateMonitorController.entropy_data[datapath_id][in_port] = [interface_entropy[key]]
            else:
                PacketRateMonitorController.entropy_data[datapath_id] = {in_port:[interface_entropy[key]]}
        return Response(status=200)

    # get the entropies of all the switches
    @route('entropy_monitor', '/entropy', methods=['GET'])
    def get_entropy(self, req, **kwargs):
        return json.dumps(PacketRateMonitorController.entropy_data)



    # update the flow count of a switch
    @route('flow_count_monitor', '/flow_count', methods=['POST'])
    def post_flow_count(self, req, **kwargs):
        data = req.json
        if data['switch_id'] in PacketRateMonitorController.flow_count_data.keys():
            for port in data['count'].keys():
                port_int = int(port)
                if port_int in PacketRateMonitorController.flow_count_data[data['switch_id']].keys():
                    PacketRateMonitorController.flow_count_data[data['switch_id']][port_int] = PacketRateMonitorController.flow_count_data[data['switch_id']][port_int][-MAX_DATAPOINTS + 1:] + [data['count'][port]]

                else:
                    PacketRateMonitorController.flow_count_data[data['switch_id']][port_int] = [data['count'][port]]
        else:
            PacketRateMonitorController.flow_count_data[data['switch_id']] = {int(k): [v] for k, v in data['count'].items()}
        return Response(status=200)

    # get the flow rates of all the switches
    @route('flow_count_monitor', '/flow_count', methods=['GET'])
    def get_flow_count(self, req, **kwargs):
        return json.dumps(PacketRateMonitorController.flow_count_data)