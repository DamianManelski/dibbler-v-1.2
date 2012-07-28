/*
 * Dibbler - a portable DHCPv6
 *
 * author: Michal Kowalczuk <michal@kowalczuk.eu>
 *
 * released under GNU GPL v2 only licence
 *
 */

#include "ClntOptAuthentication.h"
#include "ClntMsg.h"

TClntOptAuthentication::TClntOptAuthentication( char * buf,  int n, TMsg* parent)
	:TOptAuthentication(buf, n, parent)
{
}

TClntOptAuthentication::TClntOptAuthentication(TMsg* parent)
    :TOptAuthentication(parent)
{
    setRDM(0);
}

bool TClntOptAuthentication::doDuties()
{
    int ifindex = this->Parent->getIface();
    SPtr<TClntCfgIface> cfgIface = ClntCfgMgr().getIface(ifindex);
    cfgIface->setAuthenticationState(STATE_CONFIGURED);

    return false; /// @todo: why return false here?
}
