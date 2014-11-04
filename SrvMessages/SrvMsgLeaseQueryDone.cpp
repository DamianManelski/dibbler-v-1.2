/*
 * Dibbler - a portable DHCPv6
 *
 * authors: Karol Podolski <podol(at)ds.pg.gda.pl>
 *          Tomasz Mrugalski <thomson(at)klub.com.pl>
 *
 * released under GNU GPL v2 only licence
 *
 */

#include "SrvMsgLeaseQueryDone.h"
#include "Logger.h"
#include "SrvOptLQ.h"
#include "AddrClient.h"
#include "OptStatusCode.h"

TSrvMsgLeaseQueryDone::TSrvMsgLeaseQueryDone(SPtr<TSrvMsgLeaseQuery> query)
	:TSrvMsg(query->getIface(), query->getAddr(), LEASEQUERY_DONE_MSG,
			query->getTransID(), query->Bulk)
{
	// append Status code option
	Options.push_back(new TOptStatusCode(STATUSCODE_SUCCESS, "Bulk Leasequery transaction succeed.", this));
}


bool TSrvMsgLeaseQueryDone::check() {
    // this should never happen
    return true;
}

TSrvMsgLeaseQueryDone::~TSrvMsgLeaseQueryDone() {
}

unsigned long TSrvMsgLeaseQueryDone::getTimeout() {
    return 0;
}
void TSrvMsgLeaseQueryDone::doDuties() {
    IsDone = true;
}


std::string TSrvMsgLeaseQueryDone::getName() const {
    return "LEASE-QUERY-DONE";
}
