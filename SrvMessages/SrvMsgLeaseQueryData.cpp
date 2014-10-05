/*
 * Dibbler - a portable DHCPv6
 *
 * authors: Karol Podolski <podol(at)ds.pg.gda.pl>
 *          Tomasz Mrugalski <thomson(at)klub.com.pl>
 *
 * released under GNU GPL v2 only license
 *
 */

#include "SrvMsgLeaseQuery.h"
#include "SrvMsgLeaseQueryData.h"
#include "SrvCfgMgr.h"
#include "OptDUID.h"
#include "Logger.h"

TSrvMsgLeaseQueryData::TSrvMsgLeaseQueryData(SPtr<TSrvMsgLeaseQuery> query)
	:TSrvMsg(query->getIface(), query->getAddr(), LEASEQUERY_DATA_MSG,
	query->getTransID(), query->Bulk)
{
    // append CLIENT-ID
    SPtr<TOpt> opt;
    opt = query->getOption(OPTION_CLIENTID);
    if (!opt) {
        Log(Error) << "BLQ: query does not have client-id. Malformed." << LogEnd;
        IsDone = true;
        return;
    }
    Options.push_back(opt);

	// append SERVERID
	SPtr<TOptDUID> serverID;
	serverID = new TOptDUID(OPTION_SERVERID, SrvCfgMgr().getDUID(), this);
	Options.push_back((Ptr*)serverID);

    // don't do anything else
    // someone will call appendClientData() on us
    // and then call sendTCP()
}

std::string TSrvMsgLeaseQueryData::getName() const {
    return "LEASE-QUERY-DATA";
}

TSrvMsgLeaseQueryData::~TSrvMsgLeaseQueryData() {
}

void TSrvMsgLeaseQueryData::appendClientData(SPtr<TAddrClient> cli)
{
	Log(Debug) << "LQ: Appending data for client " << cli->getDUID()->getPlain() << LogEnd;

	SPtr<TSrvOptLQClientData> cliData = new TSrvOptLQClientData(this);

	SPtr<TAddrIA> ia;
	SPtr<TAddrAddr> addr;
	SPtr<TAddrPrefix> prefix;

	unsigned long nowTs = (uint32_t)time(NULL);
	unsigned long cliTs = cli->getLastTimestamp();
	unsigned long diff = nowTs - cliTs;

	Log(Debug) << "LQ: modifying the lifetimes (client last seen " << diff << "secs ago)." << LogEnd;

	// add all assigned addresses
	cli->firstIA();
	while (ia = cli->getIA()) {
		ia->firstAddr();
		while (addr = ia->getAddr()) {
			unsigned long a = addr->getPref() - diff;
			unsigned long b = addr->getValid() - diff;
			cliData->addOption(new TSrvOptIAAddress(addr->get(), a, b, this));
		}
	}

	// add all assigned prefixes
	cli->firstPD();
	while (ia = cli->getPD()) {
		ia->firstPrefix();
		while (prefix = ia->getPrefix()) {
			cliData->addOption(new TSrvOptIAPrefix(prefix->getPrefix(), prefix->getLength(), prefix->getPref(),
				prefix->getValid(), this));
		}
	}

	cliData->addOption(new TOptDUID(OPTION_CLIENTID, cli->getDUID(), this));

	// TODO: add all temporary addresses

	// add CLT_TIME
	Log(Debug) << "LQ: Adding CLT_TIME option: " << diff << " second(s)." << LogEnd;

	cliData->addOption(new TSrvOptLQClientTime(diff, this));

	Options.push_back((Ptr*)cliData);
}

