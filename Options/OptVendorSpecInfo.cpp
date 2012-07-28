/*
 * Dibbler - a portable DHCPv6
 *
 * authors: Tomasz Mrugalski <thomson@klub.com.pl>
 *          Marek Senderski <msend@o2.pl>
 *
 * released under GNU GPL v2 licence
 *
 * $Id: OptVendorSpecInfo.cpp,v 1.10 2008-03-02 19:19:43 thomson Exp $
 *
 */

#include <string.h>
#include <iostream>
#include <sstream>
#include "Portable.h"
#include "OptVendorSpecInfo.h"
#include "OptGeneric.h"
#include "DHCPConst.h"
#include "Logger.h"

#if defined(LINUX) || defined(BSD)
#include <arpa/inet.h>
#endif

TOptVendorSpecInfo::TOptVendorSpecInfo(int type, char * buf,  int n, TMsg* parent)
    :TOpt(type, parent)
{
    int optionCode = 0, optionLen = 0;
    if (n<4) {
	Log(Error) << "Unable to parse truncated vendor-spec info option." << LogEnd;
	this->Vendor = 0;
        Valid = false;
	return;
    }

    this->Vendor = readUint32(buf); // enterprise number
    buf += sizeof(uint32_t);
    n   -= sizeof(uint32_t);

    while (n>=4) {
        optionCode = readUint16(buf);
        buf += sizeof(uint16_t); n -= sizeof(uint16_t);
        optionLen  =  readUint16(buf);
        buf += sizeof(uint16_t); n -= sizeof(uint16_t);
        if (optionLen>n) {
            Log(Warning) << "Malformed vendor-spec info option. Suboption " << optionCode
                         << " truncated." << LogEnd;
            Valid = false;
            return;
        }

        SPtr<TOpt> opt = new TOptGeneric(optionCode, buf, optionLen, parent);
        addOption(opt);
        buf += optionLen;
        n   -= optionLen;
    }
    if (n) {
        Log(Warning) << "Extra " << n << " bytes, after parsing suboption " << optionCode
                     << " in vendor-spec info option." << LogEnd;
        Valid = false;
        return;
    }
    Valid = true;
}

TOptVendorSpecInfo::TOptVendorSpecInfo(int enterprise, int optionCode, 
                                       char *data, int dataLen, TMsg* parent)
    :TOpt(OPTION_VENDOR_OPTS, parent)
{
    this->Vendor = enterprise;
    if (optionCode) 
    {
        SPtr<TOptGeneric> opt = new TOptGeneric(optionCode, data, dataLen, parent);
        addOption( (Ptr*) opt);
    }
}

TOptVendorSpecInfo::~TOptVendorSpecInfo() 
{
}

size_t TOptVendorSpecInfo::getSize() {
    SPtr<TOpt> opt;
    unsigned int len = 8; // normal header(4) + enterprise(4)
    firstOption();
    while (opt = getOption()) {
        len += opt->getSize();
    }
    return len;
}

char * TOptVendorSpecInfo::storeSelf( char* buf)
{
    // option-code OPTION_VENDOR_OPTS (2 bytes long)
    buf = writeUint16(buf, OptType);

    // option-len size of total option-data
    buf = writeUint16(buf, getSize()-4);

    // enterprise-number (4 bytes long)
    buf = writeUint32(buf, this->Vendor);

    SPtr<TOpt> opt;
    firstOption();

    while (opt = getOption())
    {
        buf = opt->storeSelf(buf);
    }
    
    return buf;
}

bool TOptVendorSpecInfo::isValid()
{
    return true;
}

unsigned int TOptVendorSpecInfo::getVendor()
{
    return Vendor;
}
