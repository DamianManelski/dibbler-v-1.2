/*                                                                           
 * Dibbler - a portable DHCPv6                                               
 *                                                                           
 * authors: Tomasz Mrugalski <thomson@klub.com.pl>                           
 *          Marek Senderski <msend@o2.pl>                                    
 * changes: Michal Kowalczuk <michal@kowalczuk.eu>
 *                                                                           
 * released under GNU GPL v2 only licence                                
 *                                                                           
 */

#include "SrvMsgLeaseQueryReply.h"
#include "Logger.h"
#include "SrvOptLQ.h"
#include "OptStatusCode.h" 
#include "OptDUID.h"
#include "SrvOptIAAddress.h"
#include "SrvOptIAPrefix.h"
#include "AddrClient.h"
#include "SrvCfgMgr.h"
#include "OptVendorData.h"
#include "SrvMsgLeaseQueryData.h"
#include "SrvMsgLeaseQueryDone.h"

using namespace std;

TSrvMsgLeaseQueryReply::TSrvMsgLeaseQueryReply(SPtr<TSrvMsgLeaseQuery> query)
    :TSrvMsg(query->getIface(), query->getRemoteAddr(), LEASEQUERY_REPLY_MSG,
	     query->getTransID(),query->Bulk)
{
	
	if(!this->Bulk) {
		if (!answer(query)) {
			Log(Error) << "LQ-QUERY response generation failed." << LogEnd;
			IsDone = true;
		} else {
			Log(Debug) << "LQ-QUERY response generation successful." << LogEnd;
		}IsDone = true;
	} else {
		if (!answerBlq(query)) {
			Log(Error) << "Bulk LQ-QUERY response generation failed." << LogEnd;
		} else {
			Log(Debug) << "Bulk LQ-QUERY response generation successful." << LogEnd;
		}
		IsDone = false;
	}
}


/** 
 * 
 * 
 * @param queryMsg 
 * 
 * @return true - answer should be sent
 */
bool TSrvMsgLeaseQueryReply::answer(SPtr<TSrvMsgLeaseQuery> queryMsg) {
	int count = 0;
    SPtr<TOpt> opt;
    bool ok = true;

    Log(Info) << "LQ: Generating new LEASEQUERY_RESP message." << LogEnd;
    
    queryMsg->firstOption();
    while ( opt = queryMsg->getOption()) {
	switch (opt->getOptType()) {
	case OPTION_LQ_QUERY:
	{
	    count++;
	    SPtr<TSrvOptLQ> q = (Ptr*) opt;
	    switch (q->getQueryType()) {
	    case QUERY_BY_ADDRESS:
		ok = queryByAddress(q, queryMsg);
		break;
	    case QUERY_BY_CLIENT_ID:
		ok = queryByClientID(q, queryMsg);
		break;
	    default:
		Options.push_back(new TOptStatusCode(STATUSCODE_UNKNOWNQUERYTYPE, "Invalid Query type.", this) );
		Log(Warning) << "LQ: Invalid query type (" << q->getQueryType() << " received." << LogEnd;
		return true;
	    }
	    if (!ok) {
		Log(Warning) << "LQ: Malformed query detected." << LogEnd;
		return false;
	    }
	    break;
	}
	case OPTION_CLIENTID:
	    // copy the client-id option
	    Options.push_back(opt);
	    break;
	}

    }
    if (!count) {
	Options.push_back(new TOptStatusCode(STATUSCODE_MALFORMEDQUERY, "Required LQ_QUERY option missing.", this));
	return true;
    }

    // append SERVERID
    SPtr<TOptDUID> serverID;
    serverID = new TOptDUID(OPTION_SERVERID, SrvCfgMgr().getDUID(), this);
    Options.push_back((Ptr*)serverID);

    // allocate buffer
    this->send();

    return true;
}
/**
*
*
* @param queryMsg
*
* @return true - answer should be sent
*/
bool TSrvMsgLeaseQueryReply::answerBlq(SPtr<TSrvMsgLeaseQuery> queryMsg) {

	int count = 0;
	SPtr<TOpt> opt;
	SPtr<TSrvOptLQ> subOpt;
	bool send = false;
	bool isBlqDataExist = false;

	if (!queryMsg->Bulk)
		Log(Info) << "LQ: Generating new LEASEQUERY-REPLY message." << LogEnd;
	else
		Log(Info) << "BLQ: Generating new Bulk LEASEQUERY-REPLY message" << LogEnd;

	queryMsg->firstOption();

	while (opt = queryMsg->getOption()) {

		subOpt = (Ptr*)opt;
		if (opt->getOptType() == OPTION_CLIENTID) 
		{
			continue;
		}
		else if (opt->getOptType() == OPTION_LQ_QUERY) 
		{
			switch (subOpt->getQueryType()) {
			case QUERY_BY_ADDRESS:
				send = queryByAddress(subOpt, queryMsg);
				count++;
				break;
			case QUERY_BY_CLIENT_ID:
				send = queryByClientID(subOpt, queryMsg);
				count++;
				break;
			case QUERY_BY_LINK_ADDRESS:
				send = queryByLinkAddress(subOpt, queryMsg);
				count++;
				break;
			case QUERY_BY_RELAY_ID:
				send = queryByRelayID(subOpt, queryMsg);
				count++;
				break;
			case QUERY_BY_REMOTE_ID:
				send = queryByRemoteID(subOpt, queryMsg);
				count++;
				break;

			default:
				Options.push_back(new TOptStatusCode(STATUSCODE_UNKNOWNQUERYTYPE, "Invalid Query type.", this));
				Log(Warning) << "LQ: Invalid query type (" << subOpt->getQueryType() << " received." << LogEnd;
				send = true;
				break;
			}
			subOpt = 0;
		}
	}

	if (!count) {
		Options.push_back(new TOptStatusCode(STATUSCODE_MALFORMEDQUERY, "Required LQ_QUERY option missing.", this));
		send = true;
	}

	// append SERVERID
	SPtr<TOptDUID> serverID;
	serverID = new TOptDUID(OPTION_SERVERID, SrvCfgMgr().getDUID(), this);
	Options.push_back((Ptr*)serverID);

	if (send) {
		// allocate buffer
		this->send();
	}


	if (!isComplete) {
		SPtr<TSrvMsgLeaseQueryData> lqData;
		do{
			lqData = new TSrvMsgLeaseQueryData(queryMsg);
			if (lqData){
				isBlqDataExist = true;
				blqClntsLst.first();
				lqData->appendClientData(blqClntsLst.getFirst());
				blqClntsLst.delFirst();
				lqData->send();
				if (blqClntsLst.count() == 0)
					lqData->isComplete = true;
			}
		} while (!lqData->isComplete);
	}

	if (isBlqDataExist)
	{
		SPtr<TSrvMsgLeaseQueryDone> lqDone = new TSrvMsgLeaseQueryDone(queryMsg);
		lqDone->send();
	}
	return true;
}

bool TSrvMsgLeaseQueryReply::queryByAddress(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg) {
    SPtr<TOpt> opt;
   
    SPtr<TSrvOptIAAddress> addr = 0;
    SPtr<TIPv6Addr> link = q->getLinkAddr();
	q->firstOption();
    while ( opt = q->getOption() ) {
	if (opt->getOptType() == OPTION_IAADDR)
	    addr = (Ptr*) opt;
    }
    if (!addr) {
		Options.push_back(new TOptStatusCode(STATUSCODE_MALFORMEDQUERY, "Required IAADDR suboption missing.", this));
		isComplete = true;
		return true;
    }
	
	
	// search for client
	if (this->Bulk){
		SPtr<TAddrClient> cli;
		getAllAddrBindings(addr->getAddr());
		
		if (!this->blqClntsLst.count()) {
			Log(Warning) << "LQ: Assignment for client addr=" << addr->getAddr()->getPlain() << " not found." << LogEnd;
			Options.push_back(new TOptStatusCode(STATUSCODE_NOTCONFIGURED, "No binding for this address found.", this));
			return true;
		}
		else {
			//append first match client's binding
			blqClntsLst.first();
			cli = blqClntsLst.get();
			appendClientData(cli);

			if (blqClntsLst.count() > 1)
			{
				//remove this client from queue:
				blqClntsLst.delFirst();
				isComplete = false;
			}
		}
	} else {

		SPtr<TAddrClient> cli;
		cli = SrvAddrMgr().getClient(addr->getAddr());
		if (!cli) {
			Log(Warning) << "LQ: Assignment for client addr=" << addr->getAddr()->getPlain() << " not found." << LogEnd;
			Options.push_back(new TOptStatusCode(STATUSCODE_NOTCONFIGURED, "No binding for this address found.", this));
			return true;
		}

		appendClientData(cli);
	}

    return true;
}

bool TSrvMsgLeaseQueryReply::queryByClientID(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg) {
    SPtr<TOpt> opt;
    SPtr<TOptDUID> duidOpt = 0;
    SPtr<TDUID> duid = 0;
    //SPtr<TIPv6Addr> link = q->getLinkAddr();
    
    queryMsg->firstOption();
    while ( opt = queryMsg->getOption() ) {
	if (opt->getOptType() == OPTION_CLIENTID) {
	    duidOpt = (Ptr*) opt;
	    duid = duidOpt->getDUID();
	}
    }
    if (!duid) {
		Options.push_back( new TOptStatusCode(STATUSCODE_UNSPECFAIL, "You didn't send your ClientID.", this) );
		isComplete = true;
		return true;
    }

	// search for client
	if (this->Bulk) {
		SPtr<TAddrClient> cli;
		this->getAllClientDUIDRelatedBindings(duid);

		if (!this->blqClntsLst.count()) {
			Log(Warning) << "BLQ: Assignment for client addr=" << duid->getPlain() << " not found." << LogEnd;
			Options.push_back(new TOptStatusCode(STATUSCODE_NOTCONFIGURED, "No binding for this Client Id found.", this));
			return true;
		}
		else {
			//append first match client's binding
			blqClntsLst.first();
			cli = blqClntsLst.get();
			appendClientData(cli);

			if (blqClntsLst.count() > 1)
			{
				//remove this client from queue:
				blqClntsLst.delFirst();
				isComplete = false;
			}
		}
	}
	else {

		// search for client
		SPtr<TAddrClient> cli = SrvAddrMgr().getClient(duid);

		if (!cli) {
			Log(Warning) << "LQ: Assignment for client duid=" << duid->getPlain() << " not found." << LogEnd;
			Options.push_back(new TOptStatusCode(STATUSCODE_NOTCONFIGURED, "No binding for this DUID found.", this));
			isComplete = true;
			return true;
		}

		appendClientData(cli);
	}
    return true;
}
bool TSrvMsgLeaseQueryReply::queryByLinkAddress(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg) {
    SPtr<TOpt> opt;
	q->firstOption();
    SPtr<TSrvOptIAAddress> addr = 0;
    SPtr<TIPv6Addr> link= q->getLinkAddr();

 
    if (!link) {
        Options.push_back(new TOptStatusCode(STATUSCODE_MALFORMEDQUERY, "Required IAADDR suboption missing.", this));
		isComplete = true;
		return true;
    }

	// search for client
	if (this->Bulk) {
		SPtr<TAddrClient> cli;
		this->getAllRelayLinkAddrRelatedBindings(q->getLinkAddr());

		if (!this->blqClntsLst.count()) {
			Log(Warning) << "LQ: Assignment for client link addr=" << link->getPlain() << " not found." << LogEnd;
			Options.push_back(new TOptStatusCode(STATUSCODE_NOTCONFIGURED, "No binding for this address found.", this));
			isComplete = true;
			return true;
		}
		else {
			//append first match client's binding
			blqClntsLst.first();
			cli = blqClntsLst.get();
			appendClientData(cli);

			if (blqClntsLst.count() > 1)
			{
				//remove this client from queue:
				blqClntsLst.delFirst();
				isComplete = false;
			}
		}
	}
	else {

		// search for client
		SPtr<TAddrClient> cli = SrvAddrMgr().getClient(q->getLinkAddr());

		if (!cli) {
			Log(Warning) << "LQ: Assignment for client link addr=" << link->getPlain() << " not found." << LogEnd;
			Options.push_back(new TOptStatusCode(STATUSCODE_NOTCONFIGURED, "No binding for this address found.", this));
			isComplete = true;
			return true;
		}

		appendClientData(cli);
	}
    return true;
}

bool TSrvMsgLeaseQueryReply::queryByRemoteID(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg) {

    SPtr<TOpt> opt;
    q->firstOption();
    SPtr<TOptVendorData> remoteIdOpt = 0;
	SPtr<TAddrClient> cli;
    SPtr<TIPv6Addr> link = q->getLinkAddr();

    while ( opt = q->getOption() ) {
		if (opt->getOptType() == OPTION_REMOTE_ID)
		{
			remoteIdOpt = (Ptr*)opt;
			break;
		}
    }
    if (!remoteIdOpt) {
        Options.push_back(new TOptStatusCode(STATUSCODE_MALFORMEDQUERY, "Required RemoteId suboption missing.", this));
		isComplete = true;
		return true;
    }

	if (remoteIdOpt) {
		char * data = remoteIdOpt->getVendorData();
		Log(Debug) << "Checking exceptions database for remote id: " << data << LogEnd;
		getAllRemoteIdRelatedBindings(remoteIdOpt);
	} else {
		Log(Error) << "remoteId or link address not specified" << LogEnd;
		return false;
	}

	if (!blqClntsLst.count()) {
		Log(Warning) << "LQ: Assignment for client RemoteId not found." << LogEnd;
		//Log(Warning) << "LQ: Assignement for client RemoteId=" << remoteId->getDUID()->getPlain() << " not found." << LogEnd;
		Options.push_back(new TOptStatusCode(STATUSCODE_NOTCONFIGURED, "No binding for this remote duid found.", this));
		isComplete = true;
		return true;
	}
	else 
	{
		//append first match client's binding
		blqClntsLst.first();
		cli = blqClntsLst.get();
		appendClientData(cli);

		if (blqClntsLst.count() > 1)
		{
			//remove this client from queue:
			blqClntsLst.delFirst();
			isComplete = false;
		}
			
	}
	
    return true;
}

bool TSrvMsgLeaseQueryReply::queryByRelayID(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg) {
   
    SPtr<TOpt> opt;
    SPtr<TOptDUID> relayDuidOpt = 0;
    SPtr<TDUID> relayDuid = 0;
    SPtr<TIPv6Addr> link = q->getLinkAddr();

    q->firstOption();
    while ( opt = q->getOption() ) {
        if (opt->getOptType() == OPTION_RELAY_ID) {
            relayDuidOpt = (Ptr*) opt;
            relayDuid = relayDuidOpt->getDUID();
        }
    }
    if (!relayDuid) {
		Options.push_back( new TOptStatusCode(STATUSCODE_UNSPECFAIL, "You didn't send your relay DUID.", this) );
		isComplete = true;
		return true;
    }

	// search for client

	if (!link) {
		Options.push_back(new TOptStatusCode(STATUSCODE_UNSPECFAIL, "You didn't send your link address.", this));
		isComplete = true;
		return true;
	}

	SPtr<TAddrClient> cli;
	Log(Debug) << link->getPlain() << LogEnd;
	getAllRelayIdRelatedBindings(relayDuid);
	
	/*if (link->getPlain() == "0000:0000:0000:0000:0000")
		getAllClientDUIDRelatedBindings(duid);
	else*/


	if (!blqClntsLst.count()) {
		Log(Warning) << "BLQ: Assignment for client addr=" << relayDuid->getPlain() << " not found." << LogEnd;
		Options.push_back(new TOptStatusCode(STATUSCODE_NOTCONFIGURED, "No binding for this Relay DUID found.", this));
		return true;
	}
	else {

		blqClntsLst.first();
		cli = blqClntsLst.get();
		appendClientData(cli);

		if (blqClntsLst.count() > 1)
		{
			//remove this client from queue:
			blqClntsLst.delFirst();
			isComplete = false;
		}
	}
   
    return true;
}

void TSrvMsgLeaseQueryReply::appendClientData(SPtr<TAddrClient> cli) {

    Log(Debug) << "LQ: Appending data for client " << cli->getDUID()->getPlain() << LogEnd;

    SPtr<TSrvOptLQClientData> cliData = new TSrvOptLQClientData(this);
    
    SPtr<TAddrIA> ia;
    SPtr<TAddrAddr> addr;
    SPtr<TAddrPrefix> prefix;

    unsigned long nowTs = (uint32_t) time(NULL);
    unsigned long cliTs = cli->getLastTimestamp();
    unsigned long diff = nowTs - cliTs;

    Log(Debug) << "LQ: modifying the lifetimes (client last seen " << diff << "secs ago)." << LogEnd;

    // add all assigned addresses
    cli->firstIA();
    while ( ia = cli->getIA() ) {
	ia->firstAddr();
	while ( addr=ia->getAddr() ) {
	    unsigned long a = addr->getPref() - diff;
	    unsigned long b = addr->getValid() - diff;
	    cliData->addOption( new TSrvOptIAAddress(addr->get(), a, b, this) );
	}
    }

    // add all assigned prefixes
    cli->firstPD();
    while ( ia = cli->getPD() ) {
	ia->firstPrefix();
	while (prefix = ia->getPrefix()) {
	    cliData->addOption( new TSrvOptIAPrefix( prefix->getPrefix(), prefix->getLength(), prefix->getPref(),
						     prefix->getValid(), this));
	}
    }

    cliData->addOption(new TOptDUID(OPTION_CLIENTID, cli->getDUID(), this));

    // TODO: add all temporary addresses

    // add CLT_TIME
    Log(Debug) << "LQ: Adding CLT_TIME option: " << diff << " second(s)." << LogEnd;

    cliData->addOption( new TSrvOptLQClientTime(diff, this));

    Options.push_back((Ptr*)cliData);
}

void  TSrvMsgLeaseQueryReply::getAllClientDUIDRelatedBindings(SPtr<TDUID> opt){
	
    SPtr<TAddrClient> cli;
	SrvAddrMgr().firstClient();
	cli = SrvAddrMgr().getClient(opt);
	if (cli)
		blqClntsLst.append(cli);
}
void  TSrvMsgLeaseQueryReply::getAllRelayLinkAddrRelatedBindings(SPtr<TIPv6Addr> linkaddr) {
	
	SPtr<TAddrClient> cli;
	linkaddr->getPlain();
	SrvAddrMgr().firstClient();
	while (cli = SrvAddrMgr().getClient())
	{
		if (cli)
		{
			SPtr<TIPv6Addr> relayLinkAddr = cli->getRelayLinkAddr();
			if (relayLinkAddr)
			{
				Log(Debug) << "Looking for:" << linkaddr->getPlain() << LogEnd;
				Log(Debug) << "Actual:" << relayLinkAddr->getPlain() << LogEnd;
				if (relayLinkAddr->getPlain() == linkaddr->getPlain())
					blqClntsLst.append(cli);
			}			
		}
	}	
}

void  TSrvMsgLeaseQueryReply::getAllAddrBindings(SPtr<TIPv6Addr> addr) {
	
	SPtr<TAddrClient> cli;
	addr->getPlain();
	SrvAddrMgr().firstClient();
	cli = SrvAddrMgr().getClient(addr);
	if (cli) 
		blqClntsLst.append(cli);
}

void TSrvMsgLeaseQueryReply::getAllRemoteIdRelatedBindings(SPtr<TOptVendorData> remoteID) {
	
	SPtr<TAddrClient> cli;
	SrvAddrMgr().firstClient();
	while (cli = SrvAddrMgr().getClient())
	{
		SPtr<TOptVendorData> clntRemoteId = cli->getRemoteId();
		if (clntRemoteId) 
		{	
			if (clntRemoteId->getVendorDataPlain() == remoteID->getVendorDataPlain())
				blqClntsLst.append(cli);
		}
	}
}


bool TSrvMsgLeaseQueryReply::check() {
    // this should never happen
    return true;
}

TSrvMsgLeaseQueryReply::~TSrvMsgLeaseQueryReply() {
}

unsigned long TSrvMsgLeaseQueryReply::getTimeout() {
    return 0;
}
void TSrvMsgLeaseQueryReply::doDuties() {
    IsDone = true;
}

bool TSrvMsgLeaseQueryReply::validateMsg(SPtr<TSrvMsgLeaseQuery> queryMsg)
{
    int failCount=0;
    SPtr<TOpt> opt;
    opt = queryMsg->getOption(OPTION_LQ_QUERY);

    if (!opt) {
        Options.push_back(new TOptStatusCode(STATUSCODE_MALFORMEDQUERY, "Required LQ_QUERY option missing.", this));
        failCount++;
    }

    // copy CLIENT-ID
    opt = queryMsg->getOption(OPTION_CLIENTID);
    if (!opt) {
        Log(Error) << "LQ: query does not have client-id. Malformed." << LogEnd;
        if(!failCount) {
            Options.push_back(new TOptStatusCode(STATUSCODE_MALFORMEDQUERY, "Required LQ_QUERY option missing.", this));
        }
        IsDone = true;
        failCount++;
    }
    Options.push_back(opt);

    // append SERVERID
    SPtr<TOptDUID> serverID;
    serverID = new TOptDUID(OPTION_SERVERID, SrvCfgMgr().getDUID(), this);
    Options.push_back((Ptr*)serverID);

    if(failCount)
       return false;
    else
        return true;
}
string TSrvMsgLeaseQueryReply::getName() const {
    return "LEASE-QUERY-REPLY";
}

void TSrvMsgLeaseQueryReply::getAllRelayIdRelatedBindings(SPtr<TDUID> relayId)
{
	SPtr<TAddrClient> cli;
	SrvAddrMgr().firstClient();
	while (cli = SrvAddrMgr().getClient())
	{
		SPtr<TDUID> id = cli->getRelayId();
		if (id)
		{
			if (id->getPlain() == relayId->getPlain()){
				blqClntsLst.append(cli);
			}
		}
		else
		{
			//Log(Debug) << "RelayId searched:" << relayId->getPlain() << LogEnd;
			if (id)
				Log(Debug) << "RelayId founded:" << id->getPlain() << LogEnd;
		}
			
	}
}
