/*
 * Dibbler - a portable DHCPv6
 *
 * authors: Tomasz Mrugalski <thomson@klub.com.pl>
 *
 * released under GNU GPL v2 only licence
 *
 * $Id: ReqOpt.h,v 1.3 2008-08-29 00:07:33 thomson Exp $
 *
 */

#ifndef REQOPT_H
#define REQOPT_H

#include "Opt.h"
#include "OptIAAddress.h"
#include "OptDUID.h"
#include "OptGeneric.h"
#include "SmartPtr.h"
#include "OptVendorSpecInfo.h"
#include "Portable.h"
#include "DHCPConst.h"
#include "Logger.h"

class TReqOptAddr : public TOptIAAddress
{
public:
    TReqOptAddr(int type, SPtr<TIPv6Addr> addr, TMsg * parent);
protected:
    bool doDuties();
};



#if 0
class TReqOptRemoteId : public TOptVendorSpecInfo
{
public:
    TReqOptRemoteId(int type,char * remoteId,int enterprise,char * data, int dataLen, TMsg* parent);
    size_t getSize();
    char * storeSelf( char* buf, int queryType, int enterpriseNum);
protected:
    bool doDuties();
    char * remoreIdRqOpt;
};
#endif

#endif
