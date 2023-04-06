from ryu.app import simple_switch_13
from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import MAIN_DISPATCHER, CONFIG_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.lib.packet import ethernet
from ryu.ofproto import ofproto_v1_3, ether
from ryu.app.wsgi import ControllerBase, WSGIApplication, route
import json
import requests
from ryu.lib.packet import packet
from ryu.lib.packet import ethernet
from ryu.lib.packet import ether_types
from scapy.all import *


class PacketRateMonitor(app_manager.RyuApp):
    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]
    _CONTEXTS = {'wsgi': WSGIApplication}

    def __init__(self, *args, **kwargs):
        super(PacketRateMonitor, self).__init__(*args, **kwargs)
        wsgi = kwargs['wsgi']
        wsgi.register(PacketRateMonitorController)
        self.mac_to_port = {}

    # @set_ev_cls(ofp_event.EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    # def switch_features_handler(self, ev):
    #     datapath = ev.msg.datapath
    #     ofproto = datapath.ofproto
    #     parser = datapath.ofproto_parser

    #     # install table-miss flow entry
    #     #
    #     # We specify NO BUFFER to max_len of the output action due to
    #     # OVS bug. At this moment, if we specify a lesser number, e.g.,
    #     # 128, OVS will send Packet-In with invalid buffer_id and
    #     # truncated packet data. In that case, we cannot output packets
    #     # correctly.  The bug has been fixed in OVS v2.1.0.
    #     match = parser.OFPMatch()
    #     actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER,
    #                                       ofproto.OFPCML_NO_BUFFER)]
    #     self.add_flow(datapath, 0, match, actions)

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
        print("IT WORKSS")
        if ev.msg.msg_len < ev.msg.total_len:
            self.logger.debug("packet truncated: only %s of %s bytes",
                              ev.msg.msg_len, ev.msg.total_len)
        msg = ev.msg
        datapath = msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        in_port = msg.match['in_port']

        pkt = packet.Packet(msg.data)
        eth = pkt.get_protocols(ethernet.ethernet)[0]

        # if pkt.haslayer(IP):
        #     src_ip = pkt[IP].src
        #     dst_ip = pkt[IP].dst

        if eth.ethertype == ether_types.ETH_TYPE_LLDP:
            # ignore lldp packet
            return
        dst = eth.dst
        src = eth.src

        dpid = format(datapath.id, "d").zfill(16)
        self.mac_to_port.setdefault(dpid, {})

        self.logger.info("packet in %s %s %s %s %s %s", dpid, src, dst, src_ip, dst_ip, in_port)

        # learn a mac address to avoid FLOOD next time.
        self.mac_to_port[dpid][src] = in_port

        if dst in self.mac_to_port[dpid]:
            out_port = self.mac_to_port[dpid][dst]
        else:
            out_port = ofproto.OFPP_FLOOD

        actions = [parser.OFPActionOutput(out_port)]

        # install a flow to avoid packet_in next time
        if out_port != ofproto.OFPP_FLOOD:
            match = parser.OFPMatch(in_port=in_port, eth_dst=dst, eth_src=src)
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

    @set_ev_cls(ofp_event.EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        datapath = ev.msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser


        match = parser.OFPMatch()
        actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER,
                                          ofproto.OFPCML_NO_BUFFER)]
        self.add_flow(datapath, 0, match, actions)

        # Enable port statistics
        req = parser.OFPPortStatsRequest(datapath, 0, ofproto.OFPP_ANY)
        datapath.send_msg(req)

        # Create an OFPMatch object with no match criteria
        match = parser.OFPMatch()

        # Enable aggregate statistics
        req = parser.OFPAggregateStatsRequest(datapath, 0, cookie=0, cookie_mask=0, match=match, table_id=ofproto.OFPTT_ALL, out_port=ofproto.OFPP_ANY, out_group=ofproto.OFPG_ANY)
        datapath.send_msg(req)

    @set_ev_cls(ofp_event.EventOFPPortStatsReply, MAIN_DISPATCHER)
    def port_stats_reply_handler(self, ev):
        datapath = ev.msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        stats = ev.msg.body

        # Compute packet rates for each port
        for stat in stats:
            port_no = stat.port_no
            rx_packets = stat.rx_packets
            tx_packets = stat.tx_packets
            duration_sec = stat.duration_sec
            duration_nsec = stat.duration_nsec
            packet_rate = (rx_packets + tx_packets) / (duration_sec + duration_nsec / 1e9)

            # Store packet rate data in a dictionary
            data = { 'switch_id': datapath.id, 'port_no': port_no, 'packet_rate': packet_rate }
            self.send_packet_rate_data(data)

    def send_packet_rate_data(self, data):
        # Send the packet rate data to the Ryu REST API server
        url = 'http://127.0.0.1:8080/packet_rate_monitor/packet_rate'
        response = requests.post(url, json=data)
        if response.status_code != 200:
            self.logger.error('Failed to send packet rate data')

class PacketRateMonitorController(ControllerBase):
    def __init__(self, req, link, data, **config):
        super(PacketRateMonitorController, self).__init__(req, link, data, **config)
        self.packet_rate_data = data

    @route('packet_rate_monitor', '/packet_rate', methods=['POST'])
    def packet_rate(self, req, **kwargs):
        data = req.json
        self.packet_rate_data[data['switch_id']] = data
        return Response(status=200)

    @route('packet_rate_monitor', '/packet_rate', methods=['GET'])
    def get_packet_rate(self, req, **kwargs):
        return json.dumps(self.packet_rate_data)
