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
    :TSrvMsg(query->getIface(), query->getAddr(), LEASEQUERY_REPLY_MSG,
	     query->getTransID())
{
	
	if(!query->Bulk) {
		if (!answer(query)) {
			Log(Error) << "LQ-QUERY response generation failed." << LogEnd;
			IsDone = true;
		} else {
			Log(Debug) << "LQ-QUERY response generation successful." << LogEnd;
		}IsDone = true;
	} else {
		this->Bulk = true;
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
	//here should be code comes from old leasequery implementation
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

		//opt->firstOption();
		subOpt = (Ptr*)opt;
		if (opt->getOptType() == OPTION_CLIENTID) {
			count++;
		}
		else if (opt->getOptType() == OPTION_LQ_QUERY) {

			switch (subOpt->getQueryType()) {
			case QUERY_BY_ADDRESS:
				send = queryByAddress(subOpt, queryMsg);
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

	if (count == 1) {
		queryMsg->firstOption();
		opt = queryMsg->getOption();
		opt->firstOption();
		subOpt = (Ptr*)opt;
		send = queryByClientID(subOpt, queryMsg);
		send = true;
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
		while (!lqData->isComplete) {
			lqData = new TSrvMsgLeaseQueryData(queryMsg);
			if (lqData){
				isBlqDataExist = true;
				this->send();
			}
		}
	}

	if (isBlqDataExist){
		SPtr<TSrvMsgLeaseQueryDone> lqDone;
		lqDone = new TSrvMsgLeaseQueryDone(queryMsg);
	}
	return true;
}

bool TSrvMsgLeaseQueryReply::queryByAddress(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg) {
    SPtr<TOpt> opt;
   
    SPtr<TSrvOptIAAddress> addr = 0;
    //SPtr<TIPv6Addr> link = q->getLinkAddr();
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
    SPtr<TAddrClient> cli = SrvAddrMgr().getClient( addr->getAddr() );
    
    if (!cli) {
		Log(Warning) << "LQ: Assignement for client addr=" << addr->getAddr()->getPlain() << " not found." << LogEnd;
		Options.push_back( new TOptStatusCode(STATUSCODE_NOTCONFIGURED, "No binding for this address found.", this) );
		return true;
    }
    
    appendClientData(cli);
    return true;
}

bool TSrvMsgLeaseQueryReply::queryByClientID(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg) {
    SPtr<TOpt> opt;
    SPtr<TOptDUID> duidOpt = 0;
    SPtr<TDUID> duid = 0;
    SPtr<TIPv6Addr> link = q->getLinkAddr();
    
    q->firstOption();
    while ( opt = q->getOption() ) {
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
    SPtr<TAddrClient> cli = SrvAddrMgr().getClient( duid );
    
    if (!cli) {
		Log(Warning) << "LQ: Assignement for client duid=" << duid->getPlain() << " not found." << LogEnd;
		Options.push_back( new TOptStatusCode(STATUSCODE_NOTCONFIGURED, "No binding for this DUID found.", this) );
		isComplete = true;
		return true;
    }
    
    appendClientData(cli);
    return true;
}
bool TSrvMsgLeaseQueryReply::queryByLinkAddress(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg) {
    SPtr<TOpt> opt;
    q->firstOption();
    //SPtr<TSrvOptIAAddress> addr = 0;
    SPtr<TIPv6Addr> link = q->getLinkAddr();

 
    if (!link) {
        Options.push_back(new TOptStatusCode(STATUSCODE_MALFORMEDQUERY, "Required IAADDR suboption missing.", this));
		isComplete = true;
		return true;
    }

    // search for client
    SPtr<TAddrClient> cli = SrvAddrMgr().getClient(q->getLinkAddr());

    if (!cli) {
        Log(Warning) << "LQ: Assignement for client link addr=" << link->getPlain() << " not found." << LogEnd;
        Options.push_back( new TOptStatusCode(STATUSCODE_NOTCONFIGURED, "No binding for this address found.", this) );
		isComplete = true;
		return true;
    }

    appendClientData(cli);
    return true;
}

bool TSrvMsgLeaseQueryReply::queryByRemoteID(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg) {

    SPtr<TOpt> opt;
    q->firstOption();

    SPtr<TOptVendorData> remoteId = 0;
    //SPtr<TIPv6Addr> link = q->getLinkAddr();

    while ( opt = q->getOption() ) {
        if (opt->getOptType() != OPTION_REMOTE_ID)
            continue;
        remoteId = (Ptr*) opt;
        break;
    }
    if (!remoteId) {
        Options.push_back(new TOptStatusCode(STATUSCODE_MALFORMEDQUERY, "Required RemoteId suboption missing.", this));
		isComplete = true;
		return true;
    }


    // search for client using existing RemoteId
    //TODO: how to return bindings via RemoteId by AddrMgr ?
    SPtr<TAddrClient> cli = SrvAddrMgr().getClient();
    if (!cli) {
        Log(Warning) << "LQ: Assignement for client RemoteId=" << remoteId->getPlain() << " not found." << LogEnd;
        Options.push_back( new TOptStatusCode(STATUSCODE_NOTCONFIGURED, "No binding for this address found.", this) );
		isComplete = true;
		return true;
    }

    appendClientData(cli);
    return true;

    // algorithm:
    // search thru AddrMgr
    // for each matching client:
    //
    // if (first-match) {
    //      appendClientData(cli);
    //      sendTCP(); // send this message
    // } else {
    //      msg = new TSrvOptLeaseQueryData(queryMsg);
    //      msg->appendClientData(cli);
    //      msg->sendTCP();
    // }
    // msg = new TSrvOptLaseQueryDone(queryMsg);
    // msg->sendTCP();
    return false;
}

bool TSrvMsgLeaseQueryReply::queryByRelayID(SPtr<TSrvOptLQ> q, SPtr<TSrvMsgLeaseQuery> queryMsg) {
    /// @todo: Implement query by relay-id

    SPtr<TOpt> opt;
    SPtr<TOptDUID> relayDuidOpt = 0;
    SPtr<TDUID> duid = 0;
   // SPtr<TIPv6Addr> link = q->getLinkAddr();

    q->firstOption();
    while ( opt = q->getOption() ) {
        if (opt->getOptType() == OPTION_RELAY_ID) {
            relayDuidOpt = (Ptr*) opt;
            duid = relayDuidOpt->getDUID();
        }
    }
    if (!duid) {
		Options.push_back( new TOptStatusCode(STATUSCODE_UNSPECFAIL, "You didn't send your relay DUID.", this) );
		isComplete = true;
		return true;
    }

    // search for client by duid
    SPtr<TAddrClient> cli = SrvAddrMgr().getClient( duid );

    if (!cli) {
        Log(Warning) << "LQ: Assignement for client duid=" << duid->getPlain() << " not found." << LogEnd;
        Options.push_back( new TOptStatusCode(STATUSCODE_NOTCONFIGURED, "No binding for this Relay DUID found.", this) );
		isComplete = true;
		return true;
    }

    appendClientData(cli);
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
