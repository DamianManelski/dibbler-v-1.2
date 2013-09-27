/*
 * Dibbler - a portable DHCPv6
 *
 * authors: Tomasz Mrugalski <thomson@klub.com.pl>
 *
 * Released under GNU GPL v2 licence
 *
 */

#include <stdio.h>
#include <sstream>
#include "SocketIPv6.h"
#include "ReqTransMgr.h"
#include "ReqMsg.h"
#include "OptAddr.h"
#include "OptDUID.h"
#include "OptGeneric.h"
#include "Logger.h"
#include "ReqOpt.h"
#include "Portable.h"
#include "SmartPtr.h"
#include "OptVendorSpecInfo.h"


using namespace std;

ReqTransMgr::ReqTransMgr(TIfaceMgr * ifaceMgr)
    :CfgMgr(NULL)
{
    IfaceMgr = ifaceMgr;
}

void ReqTransMgr::SetParams(ReqCfgMgr * cfgMgr)
{
    CfgMgr = cfgMgr;
}

bool ReqTransMgr::BindSockets()
{
    if (!CfgMgr) {
        Log(Crit) << "Unable to bind sockets: no configration set." << LogEnd;
        return false;
    }

    Iface = IfaceMgr->getIfaceByName(CfgMgr->iface);
    if (!Iface) {
        Log(Crit) << "Unable to bind sockets: Interface " << CfgMgr->iface << " not found." << LogEnd;
        return false;
    }

#ifndef WIN32
    Log(Info) << "Binding socket on loopback interface." << LogEnd;

    SPtr<TIfaceIface> ptrIface;
    SPtr<TIfaceIface> loopback;
    IfaceMgr->firstIface();
    while (ptrIface=IfaceMgr->getIface()) {
        if (!ptrIface->flagLoopback()) {
            continue;
	    }
	    loopback = ptrIface;
	    break;
    }
    if (!loopback) {
	   Log(Crit) << "Loopback interface not found!" << LogEnd;
	   return false;
    }

    SPtr<TIPv6Addr> loopAddr = new TIPv6Addr("::", true);
    Log(Notice) << "Creating control (" << *loopAddr << ") socket on the " << loopback->getName() 
		<< "/" << loopback->getID() << " interface." << LogEnd;

    if (!loopback->addSocket(loopAddr,DHCPCLIENT_PORT, false, true)) {
	Log(Crit) << "Proper socket creation failed." << LogEnd;
    return false;
    }

    loopback->firstSocket();
    Socket = loopback->getSocket();
    if (!Socket) {
        Log(Crit) << "No socket found. Something is wrong." << LogEnd;
        return false;
    }
    Log(Debug) << "Socket " << Socket->getFD() << " created on the " << loopback->getFullName() << " interface." << LogEnd;


#endif

    // get link-local address
    char* llAddr = 0;
    Iface->firstLLAddress();
    llAddr=Iface->getLLAddress();
    if (!llAddr) {
    	Log(Error) << "Interface " << Iface->getFullName() << " does not have link-layer address. Weird." << LogEnd;
    	return false;
    }

    SPtr<TIPv6Addr> ll = new TIPv6Addr(llAddr);

    if (!Iface->addSocket(ll, DHCPCLIENT_PORT, true, true)) {
        Log(Crit) << "Socket creation or binding failed." << LogEnd;
        return false;
    }
    Iface->firstSocket();
    Socket = Iface->getSocket();
    if (!Socket) {
        Log(Crit) << "No socket found. Something is wrong." << LogEnd;
        return false;
    }
    Log(Debug) << "Socket " << Socket->getFD() << " created on the " << Iface->getFullName() << " interface." << LogEnd;

    return true;
}


bool ReqTransMgr::SendMsg()
{
    // TODO

    SPtr<TIPv6Addr> dstAddr;
    if (!CfgMgr->dstaddr)
    dstAddr = new TIPv6Addr("ff02::1:2", true);
    else
    dstAddr = new TIPv6Addr(CfgMgr->dstaddr, true);
    
    Log(Debug) << "Transmitting data on the " << Iface->getFullName() << " interface to "
           << dstAddr->getPlain() << " address." << LogEnd;
    TReqMsg * msg = new TReqMsg(Iface->getID(), dstAddr, LEASEQUERY_MSG);

    char buf[1024];
    int bufLen;
    memset(buf, 1024, 0xff);
    //char queryType;

    if (CfgMgr->addr) {
        Log(Debug) << "Creating ADDRESS-based query. Asking for " << CfgMgr->addr << " address." << LogEnd;
        // Address based query
        buf[0] = QUERY_BY_ADDRESS;
        // buf[1..16] - link address, leave as ::
        //memset(buf+1, 0, 16);
        bufLen = 17;

        // add new IAADDR option
        SPtr<TIPv6Addr> a = new TIPv6Addr(CfgMgr->addr, true);
        SPtr<TReqOptAddr>  optAddr = new TReqOptAddr(OPTION_IAADDR, a, msg);
        optAddr->storeSelf(buf+bufLen);
        bufLen += optAddr->getSize();
        
    } else if (CfgMgr->duid) {
        Log(Debug) << "Creating DUID-based query. Asking for " << CfgMgr->duid << " DUID." << LogEnd;
        // DUID based query
        buf[0] = QUERY_BY_CLIENT_ID;
        // buf[1..16] - link address, leave as ::
        //memset(buf+1, 0, 16);
        bufLen = 17;

        SPtr<TDUID> duid = new TDUID(CfgMgr->duid);
        SPtr<TReqOptDUID>  optDuid = new TReqOptDUID(OPTION_CLIENTID, duid, msg);
        optDuid->storeSelf(buf+bufLen);
        bufLen += optDuid->getSize();
    
    } else {
        Log(Debug) << "Creating LINK-ADDRESS-based query. Asking for " << CfgMgr->bulk << " address." << LogEnd;
        // Link-address based query
        buf[0] = QUERY_BY_LINK_ADDRESS;
        // buf[1..16] - link address, leave as ::
        memset(buf+1, 0, 16);
        bufLen = 17;

        // add new IAADDR option
        SPtr<TIPv6Addr> a = new TIPv6Addr(CfgMgr->bulk, true);
        SPtr<TReqOptAddr>  optAddr = new TReqOptAddr(OPTION_IAADDR, a, msg);
        optAddr->storeSelf(buf+bufLen);
        bufLen += optAddr->getSize();
    }


    SPtr<TDUID> clientDuid = new TDUID("00:01:00:01:0e:ec:13:db:00:02:02:02:02:02");
    SPtr<TOpt> opt = new TReqOptDUID(OPTION_CLIENTID, clientDuid, msg);
    msg->addOption(opt);

    opt = new TReqOptGeneric(OPTION_LQ_QUERY, buf, bufLen, msg);
    msg->addOption(opt);
    
    char msgbuf[1024];
    int  msgbufLen;
    memset(msgbuf, 0xff, 1024);

    msgbufLen = msg->storeSelf(msgbuf);

    Log(Debug) << msg->getSize() << "-byte long LQ_QUERY message prepared." << LogEnd;


    if (this->Socket->send(msgbuf, msgbufLen, dstAddr, DHCPSERVER_PORT)<0) {
        Log(Error) << "Message transmission failed." << LogEnd;
        return false;
    }
    Log(Info) << "LQ_QUERY message sent." << LogEnd;
    return true;
}


bool ReqTransMgr::CreateNewTCPSocket(char *dstAddr)
{
    int port=0;
    if (!CfgMgr) {
        Log(Crit) << "Unable to bind sockets: no configration set." << LogEnd;
        return false;
    }

    Iface = IfaceMgr->getIfaceByName(CfgMgr->iface);
    if (!Iface) {
        Log(Crit) << "Unable to bind sockets: Interface " << CfgMgr->iface << " not found." << LogEnd;
        return false;
    }


    // get link-local address
    char* llAddr = 0;
    Iface->firstLLAddress();
    llAddr=Iface->getLLAddress();

    if (!llAddr) {
        Log(Error) << "Interface " << Iface->getFullName() << " does not have link-layer address. Weird." << LogEnd;
        return false;
    }

    Log(Info) << "llAddr:"<< llAddr << LogEnd;
    Log(Info) << "dstAddr:"<< dstAddr << LogEnd;
   // SPtr<TIPv6Addr> ll = new TIPv6Addr(llAddr);
    SPtr<TIPv6Addr> gl = new TIPv6Addr();


    Log(Crit) << "TCP Socket creation or binding failed (link-local address)." << LogEnd;
    Log(Info) << "Trying on global address..." << LogEnd;
    //get global address
    Iface->firstGlobalAddr();

    gl=Iface->getGlobalAddr();

    if(!gl) {
        Log(Error) << "Interface " << Iface->getFullName() << " does not have global address. Weird." << LogEnd;
        return false;
    }

    SPtr<TIPv6Addr> dstAddrTmp = new TIPv6Addr(dstAddr,true);

    if(!Iface->addTcpSocket(dstAddrTmp,port,0)) {
        Log(Crit) << "TCP Socket creation or binding failed (global address)." << LogEnd;
        return false;
    } else {
        Log(Crit) << "TCP Socket creation or binding no global address ok" << LogEnd;
    }


    /*

    if (!Iface->addTcpSocket(ll,port)) {
        Log(Crit) << "TCP Socket creation or binding failed (link-local address)." << LogEnd;
        Log(Info) << "Trying on global address..." << LogEnd;
        //get global address
        Iface->firstGlobalAddr();

        gl=Iface->getGlobalAddr();

        if(!gl) {
            Log(Error) << "Interface " << Iface->getFullName() << " does not have global address. Weird." << LogEnd;
            return false;
        }

        if(!Iface->addTcpSocket(gl,port)) {
            Log(Crit) << "TCP Socket creation or binding failed (global address)." << LogEnd;
            return false;
        } else {
            Log(Crit) << "TCP Socket creation or binding no global address ok" << LogEnd;
        }

    } else {
        Log(Crit) << "TCP Socket creation or binding no link-local address ok" << LogEnd;
    }
    */

    Iface->firstSocket();
    Socket = Iface->getSocket();
    if (!Socket) {
        Log(Crit) << "No socket found. Something is wrong." << LogEnd;
        return false;
    }
    Log(Debug) << "Socket " << Socket->getFD() << " created on the " << Iface->getFullName() << " interface." << LogEnd;

    return true;

}


bool ReqTransMgr::SendTcpMsg()
{

    // bulk leasequery assumed five types of queries:
	//by Address
    //by ClientId - include DUID option
    //by Relay Id
    //Link Address
    //by Remote Id

    SPtr<TIPv6Addr> dstAddr;
    if (!CfgMgr->dstaddr) {
        dstAddr = new TIPv6Addr("ff02::1:2", true);
        Log(Debug) << "Destination addres of DHCP server not specified. Using:"<<dstAddr->getPlain() <<endl;
    }  else {
        dstAddr = new TIPv6Addr(CfgMgr->dstaddr, true);
    }

    Log(Debug) << "Transmitting data on the " << Iface->getFullName() << " interface to "
           << dstAddr->getPlain() << " address by tcp protocol." << LogEnd;


    char buf[1024];
    int bufLen;
   // char queryType;
    bufLen= sizeof(buf);
    memset(buf, 1024, 0xff);

    // nedded to create DHCP over TCP msg
    TReqMsg * msg = new TReqMsg(Iface->getID(), dstAddr, buf, LEASEQUERY_MSG,bufLen);

    switch (CfgMgr->queryType) {

    case QUERY_BY_ADDRESS:
        if (CfgMgr->addr) {
            Log(Debug) << "Creating ADDRESS-based query. Asking for " << CfgMgr->addr << " address." << LogEnd;
            // Address based query
            buf[0] = QUERY_BY_ADDRESS;
            // buf[1..16] - link address, leave as ::
            memset(buf+1, 0, 16);
            bufLen = 17;

            // add new IAADDR option
            SPtr<TIPv6Addr> a = new TIPv6Addr(CfgMgr->addr, true);
            SPtr<TReqOptAddr>  optAddr = new TReqOptAddr(OPTION_IAADDR, a, msg);
            optAddr->storeSelf(buf+bufLen);
            bufLen += optAddr->getSize();

        } else {
            Log(Debug) << "Cannot creating LinkAddr-based query for " << CfgMgr->addr << " address." <<LogEnd;
            return false;
        }

    break;

    case QUERY_BY_LINK_ADDRESS:
        if(CfgMgr->linkAddr) {

            Log(Debug) << "Creating LINK-ADDRESS-based query. Asking for " << CfgMgr->linkAddr << " address." << LogEnd;
            // Link-address based query
            buf[0] = QUERY_BY_LINK_ADDRESS;
            // buf[1..16] - link address, leave as ::
            memset(buf+1, 0, 16);
            bufLen = 17;
        } else {
            Log(Debug) << "Cannot creating LinkAddr-based query for " << CfgMgr->remoteId << " link address." << "It's not present in the server" <<LogEnd;
            return false;
        }
    break;

    case QUERY_BY_CLIENT_ID:

        if (CfgMgr->clientId) {
            Log(Debug) << "Creating ClientId-based query. Asking for " << CfgMgr->clientId << " RelayId." << LogEnd;
            // RelayId-based query
            buf[0] = QUERY_BY_CLIENT_ID;
            // buf[1..16] - link address, leave as ::
            memset(buf+1, 0, 16);
            bufLen = 17;

            // add new OPTION_CLIENT_ID option
            SPtr<TDUID> duid = new TDUID(CfgMgr->duid);
            SPtr<TReqOptDUID>  optDuid = new TReqOptDUID(OPTION_CLIENTID, duid, msg);
            optDuid->storeSelf(buf+bufLen);
            bufLen += optDuid->getSize();

        } else {
            Log(Debug) << "Cannot creating ClientId-based query for " << CfgMgr->clientId << " RelayId." << "It's not present in the server" <<LogEnd;
            return false;
        }
    break;

    case QUERY_BY_RELAY_ID:
        if (CfgMgr->relayId) {
            Log(Debug) << "Creating RelayId-based query. Asking for " << CfgMgr->relayId << " RelayId." << LogEnd;
            // RelayId-based query
            buf[0] = QUERY_BY_RELAY_ID;
            // buf[1..16] - link address, leave as ::
            //memset(buf+1, 16, 0);
			memset(buf+1, 0, 16);
            bufLen = 17;

            // add new OPTION_RELAY_ID option
            SPtr<TDUID> duid = new TDUID(CfgMgr->duid);
            SPtr<TReqOptRelayId>  optRelayId = new TReqOptRelayId(OPTION_RELAY_ID, duid, msg);//bufLen=optLen ?
            optRelayId->storeSelf(buf+bufLen);
            bufLen += optRelayId->getSize();
        } else {
            Log(Debug) << "Cannot creating RelayId-based query for " << CfgMgr->relayId << " RelayId." << "It's not present in the server" <<LogEnd;
            return false;
        }
    break;

    case QUERY_BY_REMOTE_ID:
        if (CfgMgr->remoteId && CfgMgr->enterpriseNumber) {
            Log(Debug) << "Creating RemoteId-based query. Asking for " << CfgMgr->remoteId << " RelayId." << LogEnd;
            // RelayId-based query
            buf[0] = QUERY_BY_REMOTE_ID;
            // buf[1..16] - link address, leave as ::
            //memset(buf+1, 16, 0);
			memset(buf+1, 0, 16);
            bufLen = 17;
            int dataLen = 5; // option-len: 4+ the length, in octets, of the remote-id field. Minimum opt-len is 5 octets
            char data[5];
            // add new OPTION_REMOTE_ID option

            // TReqOptRemoteId(int type,char * remoteId,int enterprise,char * data, int dataLen, TMsg* parent);
            SPtr<TReqOptRemoteId> optRemoteId = new TReqOptRemoteId(OPTION_REMOTE_ID,CfgMgr->remoteId, CfgMgr->enterpriseNumber,data, dataLen, msg);
            optRemoteId->storeSelf(buf+bufLen,CfgMgr->queryType,CfgMgr->enterpriseNumber);
            bufLen += optRemoteId->getSize();
        } else {
            if(!CfgMgr->remoteId  ) {
                Log(Debug) << "Cannot creating RemoteId-based query for " << CfgMgr->remoteId << " remote-id." << "It's not present in the server/requestor" <<LogEnd;
                return false;
            }
            if(!CfgMgr->enterpriseNumber) {
                Log(Debug) << "Cannot creating RemoteId-based query for " << CfgMgr->remoteId << " entreprise number." << "It's not present in the server/requestor" <<LogEnd;
                return false;
            }
        }
    break;

    }

    // is it use as link - layer adress ??
    SPtr<TDUID> clientDuid = new TDUID("00:01:00:01:0e:ec:13:db:00:02:02:02:02:02");

    SPtr<TOpt> opt = new TReqOptDUID(OPTION_CLIENTID, clientDuid, msg);
    msg->addOption(opt);

    opt = new TReqOptGeneric(OPTION_LQ_QUERY, buf, bufLen, msg);
    msg->addOption(opt);

    char msgbuf[1024];
    int  msgbufLen;
    memset(msgbuf, 0xff, 1024);

    msgbufLen = msg->storeSelf(msgbuf);

    Log(Debug) << msg->getSize() << "-byte long LQ_QUERY message prepared." << LogEnd;

    unsigned short tmpl=0;
    int pos=0;
    for(pos=0;pos<10;pos++) {
        tmpl = msgbuf[pos];
        Log(Debug) << "pos"<<pos<<":"<<tmpl <<LogEnd;
        tmpl=0;
    }

    if (this->Socket->send_tcp(msgbuf, msgbufLen, dstAddr, DHCPSERVER_PORT)<0) {
        Log(Error) << "Message transmission failed." << LogEnd;
        return false;
    }

     Log(Info) << "LQ_QUERY message has been send." << LogEnd;
    return true;
}


bool ReqTransMgr::WaitForRsp()
{
    char buf[1024];
    int bufLen = 1024, stype;
    memset(buf, 0, bufLen);
    SPtr<TIPv6Addr> sender = new TIPv6Addr();
    int sockFD;
    Log(Debug) << "Waiting " << CfgMgr->timeout << " seconds for reply reception." << LogEnd;

    stype = getsOpt(Socket->getFD());
    if(stype != -1) {
        if (stype==SOCK_STREAM) {
            sockFD = this->IfaceMgr->select(CfgMgr->timeout, buf, bufLen, sender,true);
        } else if (stype==SOCK_DGRAM)  {
            sockFD = this->IfaceMgr->select(CfgMgr->timeout, buf, bufLen, sender);
        }
    }

    Log(Debug) << "Returned socketID=" << sockFD << LogEnd;
    if (sockFD>0) {
        Log(Info) << "Received " << bufLen << " bytes response." << LogEnd;
        PrintRsp(buf, bufLen);
    } else {
        Log(Error) << "Response not received. Timeout or socket error." << LogEnd;
        return false;
    }
    
    return true;
}


void ReqTransMgr::PrintRsp(char * buf, int bufLen)
{
    if (bufLen < 4) {
        Log(Error) << "Unable to print message: truncated (min. len=4 required)." << LogEnd;
    }
    
    int msgType = buf[0];
    int transId = buf[1] + 256*buf[2] + 256*256*buf[3];

    Log(Info) << "MsgType: " << msgType << ", transID=0x" << hex << transId << dec << LogEnd;

    ParseOpts(msgType, 0, buf+4, bufLen-4);
}

void ReqTransMgr::PrintTcpRsp(char *buf, int bufLen)
{
    if (bufLen < 6) {
        Log(Error) << "Unable to print message: truncated (min. len=6 required)." << LogEnd;
    }

    int msgSize = buf[0]*256 + buf[0];
    int msgType = buf[2];
    int transId = buf[1]*256*256 + 256*buf[2] + buf[3];

    Log(Info) << "MsgType: " << msgType << ", transID=0x" << hex << transId << dec << LogEnd;

    ParseOpts(msgType, 0, buf+6, bufLen-6);
}

bool ReqTransMgr::ParseOpts(int msgType, int recurseLevel, char * buf, int bufLen)
{
    std::ostringstream o;
    for (int i=0; i<recurseLevel; i++)
	o << "  ";
    string linePrefix = o.str();

    int pos = 0;
    SPtr<TOpt> ptr;
    bool print = true;
    while (pos<bufLen) {
	if (pos+4>bufLen) {
	    Log(Error) << linePrefix << "Message " << msgType << " truncated. There are " << (bufLen-pos) 
		       << " bytes left to parse. Bytes ignored." << LogEnd;
	    return false;
	}
	unsigned short code = readUint16(buf+pos);
	pos += sizeof(uint16_t);
	unsigned short length = readUint16(buf+pos);
	pos += sizeof(uint16_t);
	if (pos+length>bufLen) {
	    Log(Error) << linePrefix << "Truncated option (type=" << code << ", len=" << length 
		       << " received in message << " << msgType << ". Option ignored." << LogEnd;
	    pos += length;
	    continue;
	}
	
	if (!allowOptInMsg(msgType,code)) {
	    Log(Warning) << linePrefix << "Invalid option received: Option " << code << " not allowed in message type " 
			 << msgType << ". Ignored." << LogEnd;
	    pos+=length;
	    continue;
	}
	if (!recurseLevel && !allowOptInOpt(msgType,0,code)) {
	    Log(Warning) << "Invalid option received: Option " << code << " not allowed in message type "<< msgType 
			 << " as a base option (as suboption only permitted). Ignored." << LogEnd;
	        pos+=length;
	        continue;
	}
	
        string name, o;
        o = "";
        name = "";
	
	switch (code) {
        case OPTION_STATUS_CODE:
	{
	    name ="Status Code";
	    unsigned int st = buf[pos]*256 + buf[pos+1];
	    
	    char *Message = new char[length+10];
	    memcpy(Message,buf+pos+2,length-2);
	    sprintf(Message+length-2, "(%d)", st);
	    o = string(Message);
	    delete [] Message;
	    break;
	}
	case OPTION_CLIENTID:
	    name = "ClientID";
	    o = BinToString(buf+pos, length);
	    break;
	case OPTION_SERVERID:
	    name = "ServerID";
        o = BinToString(buf+pos, length);
	    break;
	case OPTION_LQ_QUERY:
	    name = "LQ Query Option";
	    break;
	case OPTION_CLIENT_DATA:
	    name = "LQ Client Data Option";
	    Log(Info) << linePrefix << "Option " << name << "(code=" << code << "), len=" << length << LogEnd;
	    ParseOpts(msgType, recurseLevel+1, buf+pos, length);
	    print = false;
	    break;
	case OPTION_CLT_TIME:
	{
            name = "LQ Client Last Transmission Time";
	    unsigned int t = readUint32(buf+pos);
	    ostringstream out;
	    out << t << " second(s)";
	    o = out.str();
	    break;
	}
	case OPTION_LQ_RELAY_DATA:
            name = "LQ Relay Data";
	    break;
	case OPTION_LQ_CLIENT_LINK:
            name = "LQ Client Link";
	    break;
	case OPTION_IAADDR:
	{
	    TIPv6Addr * addr = new TIPv6Addr(buf+pos, false);
	    unsigned int pref  = readUint32(buf+pos+16);
	    unsigned int valid = readUint32(buf+pos+20);
	    name = "IAADDR";
	    ostringstream out;
	    out << "addr=" << addr->getPlain() << ", pref=" << pref << ", valid=" << valid;
	    o = out.str();
	    break;
	}

	default:
	    break;
	}
	if (print)
	    Log(Info) << linePrefix << "Option " << name << "(code=" << code << "), len=" << length << ": " << o << LogEnd;
	print = true;
        pos+=length;
    }
    
    return true;
}


string ReqTransMgr::BinToString(char * buf, int bufLen)
{
    std::ostringstream o;
    o << setfill('0');
    for (int i=0; i<bufLen; i++) {
	o << setw(2) << hex << (unsigned int)buf[i];
	if (i+1!=bufLen) {
	    o << ":";
	}
    }

    return o.str();
}

bool ReqTransMgr::RetryConnection()
{
    if (ReqTransMgr::SendTcpMsg()==false) {
        ReqTransMgr::SendTcpMsg();
        Log(Debug) << "Retrying TCP connection with address:" << CfgMgr->addr << " address." << LogEnd;
        return true;
    } else {
        return false;
    }
}

void ReqTransMgr::TerminateTcpConn()
{

    Log(Debug) << "Closing conection..." << LogEnd;
    int how, sockId;
    how=2;
    SPtr<TIfaceSocket> ptr;

    Iface->firstSocket();
    ptr = Iface->getSocket();
    this->Socket->terminate_tcp(ptr->getFD(),how);

}

bool ReqTransMgr::ValidateMsg(char * msgBuf)
{
    int i=0;
    for (i=0;i< 4 ; i++ ) {
        //msgBuf[0]

    }
}
int ReqTransMgr::GetQueryType()
{
    return CfgMgr->queryType;
}
