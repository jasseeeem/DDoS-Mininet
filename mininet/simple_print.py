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


class PacketRateMonitor(app_manager.RyuApp):
    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]
    _CONTEXTS = {'wsgi': WSGIApplication}

    def __init__(self, *args, **kwargs):
        super(PacketRateMonitor, self).__init__(*args, **kwargs)
        wsgi = kwargs['wsgi']
        wsgi.register(PacketRateMonitorController)
        self.mac_to_port = {}
        self.packet_count = {}

        # Start the periodic stats request timer
        self.monitor_thread = hub.spawn(self._monitor)

    def _monitor(self):
        while True:
            switches = topo_api.get_switch(self, None)
            for switch in switches:
                self._request_port_stats(switch.dp)
            hub.sleep(5) # Request port stats every 5 seconds

    def _request_port_stats(self, datapath):
        parser = datapath.ofproto_parser
        print(f"🟡\tRequesting Stats from Switch {datapath.id}")
        # Iterate through all ports and send a port stats request message for each port
        for port in datapath.ports.keys():
            print(f"\tRequesting from port {port}")
            req = parser.OFPPortStatsRequest(datapath, 0, port)
            datapath.send_msg(req)

    def add_flow(self, datapath, priority, match, actions, buffer_id=None):
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS,
                                             actions)]
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
        self.logger.info("packet in %s %s %s %s", dpid, src_mac, dst_mac, in_port)

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
        # send them to the controller
        actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER,
                                          ofproto.OFPCML_NO_BUFFER)]
        self.add_flow(datapath, 0, match, actions)

        # Enable port statistics for the switch
        req = parser.OFPPortStatsRequest(datapath, 0, ofproto.OFPP_ANY)
        datapath.send_msg(req)

        # Create an OFPMatch object with no match criteria
        match = parser.OFPMatch()

        # Enable aggregate statistics to retrieve statistics about the entire switch, such as the number of packets and bytes processed by the switch
        req = parser.OFPAggregateStatsRequest(datapath, 0, cookie=0, cookie_mask=0, match=match, table_id=ofproto.OFPTT_ALL, out_port=ofproto.OFPP_ANY, out_group=ofproto.OFPG_ANY)
        
        self.switches[datapath.id] = datapath

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
            data = { 'switch_id': datapath.id, 'port_no': port_no, 'num_packets': rx_packets, 'time': duration_sec + duration_nsec / 1e9}
            self.send_packet_rate_data(data)

        self.send_entropy_data(self.packet_count)


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

# REST API controller class
class PacketRateMonitorController(ControllerBase):
    # store as class attribute so that it is not reset after every API call (which is what happens if put inside __init__)
    packet_rate_data = {}
    entropy_data = {}

    # When a new HTTP request is received, the web framework instantiates a new instance of the controller class to handle the request. 
    # The __init__ method is called during the instantiation process to initialize the attributes of the new instance. 
    # The attributes of the previous instances are not preserved since they are specific to that instance and will be destroyed when the instance is no longer needed.
    def __init__(self, req, link, data, **config):
        super(PacketRateMonitorController, self).__init__(req, link, data, **config)
        # self.packet_rate = data 

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
            PacketRateMonitorController.packet_rate_data[data['switch_id']][data['port_no']] = {'curr_packet_rate': packet_diff / time_diff, 'prev_num_packets': data['num_packets'], 'prev_time': data['time']}
        else:
            PacketRateMonitorController.packet_rate_data[data['switch_id']] = {data['port_no']: {'curr_packet_rate': data['num_packets'] / data['time'], 'prev_num_packets': data['num_packets'], 'prev_time': data['time']}}
        return Response(status=200)
    


    # update the entropy of a switch
    @route('entropy_monitor', '/entropy', methods=['POST'])
    def post_entropy(self, req, **kwargs):
        data = req.json
        total_packets = sum(data.values())
        interface_entropy = {}
        
        for key in data:
            [datapath_id, in_port, src, dst] = key.split("-")
            probability = data[key] / total_packets
            interface_entropy['-'.join([datapath_id, in_port])] = interface_entropy.get('-'.join([datapath_id, in_port]), 0) - probability * log2(probability)

        for key in interface_entropy:
            [datapath_id, in_port] = key.split("-")
            PacketRateMonitorController.entropy_data[key] = interface_entropy[key]


    # get the entropies of all the switches
    @route('entropy_monitor', '/entropy', methods=['GET'])
    def get_entropy(self, req, **kwargs):
        return json.dumps(PacketRateMonitorController.entropy_data)


    # get the packet rates of all switches
    @route('packet_rate_monitor', '/packet_rate', methods=['GET'])
    def get_packet_rate(self, req, **kwargs):
        return json.dumps(PacketRateMonitorController.packet_rate_data)
