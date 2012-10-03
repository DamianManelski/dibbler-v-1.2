/*
 * Dibbler - a portable DHCPv6
 *
 * authors: Tomasz Mrugalski <thomson@klub.com.pl>
 *
 * Released under GNU GPL v2 licence
 *
 * $Id: ReqMsg.cpp,v 1.2 2007-12-03 16:59:17 thomson Exp $
 */

#include <string>
#include "ReqMsg.h"
#include "DHCPConst.h"

using namespace std;

TReqMsg::TReqMsg(int iface, SPtr<TIPv6Addr> addr, int msgType)
    :TMsg(iface, addr, msgType)
{

}
    // used to create TMsg object based on received char[] data
TReqMsg::TReqMsg(int iface, SPtr<TIPv6Addr> addr, char* &buf, int &bufSize)
    :TMsg(iface, addr, buf, bufSize)
{

}

//bulk
TReqMsg::TReqMsg(int iface, SPtr<TIPv6Addr> addr, int msgSize, int msgType):TMsg (iface, addr, msgSize, msgType,transID)
{

}

void TReqMsg::validateLQ()
{

}

void TReqMsg::isComletion()
{

}

void TReqMsg::multipleQuery()
{

}



//this->setAttributes(iface,addr,msgSize,msgType,transID);
string TReqMsg::getName()
{
    switch (MsgType) {
    case LEASEQUERY_MSG:
        return "LEASEQUERY";
    case LEASEQUERY_REPLY_MSG:
        return "LEASEQUERY_RSP";
    default:
        return "unknown";
    }
}
std::string TReqMsg::getName() const
{
    switch (MsgType) {
    case LEASEQUERY_MSG:
        return "LEASEQUERY";    
    case LEASEQUERY_REPLY_MSG:
        return "LEASEQUERY_RSP";
    default:
        return "unknown";
    }
}

int TReqMsg::getReqMsgType()
{
    return ReqMsgType;
}

void TReqMsg::addOption(SPtr<TOpt> opt)
{
    Options.push_back(opt);
}

