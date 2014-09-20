/*
 * Dibbler - a portable DHCPv6
 *
 * authors: Tomasz Mrugalski <thomson@klub.com.pl>
 *
 * released under GNU GPL v2 only licence
 *
 * $Id: SrvMsgLeaseQueryReply.h,v 1.3 2008-08-29 00:07:35 thomson Exp $
 *
 */

#ifndef SRVMSGLEASEQUERYREPLY_H
#define SRVMSGLEASEQUERYREPLY_H

#include "SrvMsg.h"
#include "SrvMsgLeaseQuery.h"
#include "Logger.h"
#include "SrvOptLQ.h"
#include "AddrClient.h"

class TSrvMsgLeaseQueryReply : public TSrvMsg
{
  public:
	bool isComplete;
    TSrvMsgLeaseQueryReply(SPtr<TSrvMsgLeaseQuery> query);

    bool queryByAddress(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg);
    bool queryByClientID(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg);
    bool queryByLinkAddress(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg);
    bool queryByRemoteID(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg);
    bool queryByRelayID(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg);
    void appendClientData(SPtr<TAddrClient> cli);

	void  getAllClientDUIDRelatedBindings(SPtr<TDUID> opt, SPtr<TIPv6Addr> linkaddr = NULL);
	void  getAllRemoteIdRelatedBindings(SPtr<TOptVendorData> remoteID);
	void  getAllRelayIdRelatedBindings(SPtr<TDUID> relayId);
	void  getAllRelayLinkAddrRelatedBindings(SPtr<TIPv6Addr> linkaddr);
	void  getAllAddrBindings(SPtr<TIPv6Addr> addr);
	
    bool answer(SPtr<TSrvMsgLeaseQuery> query);
	bool answerBlq(SPtr<TSrvMsgLeaseQuery> query);
    bool check();
    void doDuties();
    bool validateMsg(SPtr<TSrvMsgLeaseQuery> queryMsg);
    unsigned long getTimeout();
    std::string getName() const;
    ~TSrvMsgLeaseQueryReply();

	//Store ptr to client related with the same parameter (for example the same DUID)
	List(TAddrClient) blqClntsLst;
	List(TIPv6Addr) blqClntAddrLst;
};

#endif /* SRVMSGLEASEQUERYREPLY_H */
