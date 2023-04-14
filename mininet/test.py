from ryu.app import rest

class MyRestApp(rest.RestApiApp):
    def get_flow_stats(self, datapath_id, port_no):
        url = '/stats/flow?datapath_id={}&out_port={}'.format(datapath_id, port_no)
        response = self.rest_api.get(url)
        flow_stats = response.json()['body']
        port_flow_stats = [flow for flow in flow_stats if flow['match']['in_port'] == port_no]
        return port_flow_stats