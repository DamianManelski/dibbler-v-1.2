import ipaddr
from ipaddr import *
from ipcalc import *

class SubNetCalc():
    """Class represents IPv6 address and related methods"""

    def __init__(self,ipAddr):
        self.ipAddr = ipaddr.IPv6Address(ipAddr)

        #TODO: should be parameter 
        tmpPrefix = '128'

        self.prefix = "/"+tmpPrefix
        self.allHostsNumber

    def getIp(self):
        return this.ipAddr

    def showAdresses(self):

        #just enjoy and check couple methods below:
        print ('\n')
        print("Calculating properties for:",self.ipAddr)
        self.netH = ipaddr.ip_network(self.ipAddr)
        print("Exploded:" ,self.ipAddr._explode_shorthand_ip_string())
        print("Masked",self.netH.masked())
        print("Host numbers in network is:",self.netH)
        interface = IPv6Interface(self.ipAddr)
        print("Hostmask:",interface.with_hostmask)
        print("Netmask:",interface.with_netmask)
        print("Broadcast:",interface.subnet())
        print("Max prefx len:",self.ipAddr.max_prefixlen)
        self.netH.subnet(64,128)

    def calculateNetwork(self):
        #calculate whole scope using address and prefix
        net6 = ipaddress.ip_network(self.ipAddr+self.prefix)
        self.allHostsNumber = net6.num_addresses
    

    
