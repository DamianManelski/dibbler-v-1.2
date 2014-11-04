/*
 * Dibbler - a portable DHCPv6
 *
 * author:  Karol Podolski <podol(at)ds.pg.gda.pl>
 * changes: Tomasz Mrugalski <thomson(at)klub.com.pl>
 *
 * released under GNU GPL v2 only licence
 *
 */

#ifndef _SRVMSGLEASEQUERYDATA_H
#define	_SRVMSGLEASEQUERYDATA_H

#include "SrvMsg.h"
#include "SrvMsgLeaseQueryReply.h"
#include "Logger.h"
#include "SrvOptLQ.h"
#include "AddrClient.h"

class TSrvMsgLeaseQueryData : public TSrvMsg
{
	//friend class TSrvMsgLeaseQueryReply;

  public:
    TSrvMsgLeaseQueryData(SPtr<TSrvMsgLeaseQuery> query);

	bool isComplete = false;
    bool check() {return true; };
    void doDuties() { };
    std::string getName() const;
	void appendClientData(SPtr<TAddrClient> cli);
    ~TSrvMsgLeaseQueryData();
};

#endif	/* _SRVMSGLEASEQUERYDATA_H */
