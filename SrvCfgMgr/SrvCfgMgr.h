/*
 * Dibbler - a portable DHCPv6
 *
 * authors: Tomasz Mrugalski <thomson@klub.com.pl>
 *          Marek Senderski <msend@o2.pl>
 * changes: Michal Kowalczuk <michal@kowalczuk.eu>
 *          Nguyen Vinh Nghiem
 *
 * released under GNU GPL v2 only licence
 *
 */

class TSrvCfgMgr;

#ifndef SRVCONFMGR_H
#define SRVCONFMGR_H
#include "SmartPtr.h"
#include "SrvCfgIface.h"
#include "SrvIfaceMgr.h"
#include "CfgMgr.h"
#include "DHCPConst.h"
#include "Container.h"
#include "DUID.h"
#include "KeyList.h"
#include "SrvCfgClientClass.h"


#define SrvCfgMgr() (TSrvCfgMgr::instance())

class SrvParser;

class TSrvCfgMgr : public TCfgMgr
{
public:
    friend std::ostream & operator<<(std::ostream &strum, TSrvCfgMgr &x);

    static void instanceCreate(const std::string& cfgFile, const std::string& xmlDumpFile);
    static TSrvCfgMgr &instance();

    bool parseConfigFile(const std::string& cfgFile);

    //Interfaces acccess methods
    void firstIface();
    SPtr<TSrvCfgIface> getIface();
    SPtr<TSrvCfgIface> getIfaceByID(int iface);
    long countIface();
    void addIface(SPtr<TSrvCfgIface> iface);
    void makeInactiveIface(int ifindex, bool inactive);
    int inactiveIfacesCnt();
    SPtr<TSrvCfgIface> checkInactiveIfaces();

    void dump();

    bool setupRelay(SPtr<TSrvCfgIface> cfgIface);

    //Address assignment connected methods
    void setCounters();
    long countAvailAddrs(SPtr<TDUID> clntDuid, SPtr<TIPv6Addr> clntAddr, int iface);
    SPtr<TSrvCfgAddrClass> getClassByAddr(int iface, SPtr<TIPv6Addr> addr);
    SPtr<TSrvCfgPD> getClassByPrefix(int iface, SPtr<TIPv6Addr> prefix);
    SPtr<TIPv6Addr> getRandomAddr(SPtr<TDUID> duid, SPtr<TIPv6Addr> clntAddr, int iface);
    // bool isClntSupported(SPtr<TDUID> duid, SPtr<TIPv6Addr> clntAddr, int iface);
    bool isClntSupported(/*SPtr<TDUID> duid, SPtr<TIPv6Addr> clntAddr, int iface,*/ SPtr<TSrvMsg> msg);

    // prefix-related
    bool incrPrefixCount(int iface, SPtr<TIPv6Addr> prefix);
    bool decrPrefixCount(int iface, SPtr<TIPv6Addr> prefix);

    // class' usage management
    void delClntAddr(int iface, SPtr<TIPv6Addr> addr);
    void addClntAddr(int iface, SPtr<TIPv6Addr> addr);

    bool isIAAddrSupported(int iface, SPtr<TIPv6Addr> addr);
    bool isTAAddrSupported(int iface, SPtr<TIPv6Addr> addr);

    void addTAAddr(int iface);
    void delTAAddr(int iface);

    bool addrReserved(SPtr<TIPv6Addr> addr);
    bool prefixReserved(SPtr<TIPv6Addr> prefix);

    bool isDone();
    virtual ~TSrvCfgMgr();
    bool setGlobalOptions(SPtr<TSrvParsGlobalOpt> opt);

    // configuration parameters
    std::string getWorkdir();
    bool stateless();
    bool inactiveMode();
    bool guessMode();
    ESrvIfaceIdOrder getInterfaceIDOrder();
    int getCacheSize();
    bool reconfigureSupport();

    void setDDNSAddress(SPtr<TIPv6Addr> ddnsAddress);
    SPtr<TIPv6Addr> getDDNSAddress(int iface);

    // Bulk-LeaseQuery
    void bulkLQAccept(bool enabled);
    void bulkLQTcpPort(unsigned short portNumber);
    void bulkLQMaxConns(unsigned int maxConnections);
    void bulkLQTimeout(unsigned int timeout);

    //Authentication
#ifndef MOD_DISABLE_AUTH
    SPtr<KeyList> AuthKeys;
    unsigned int getAuthLifetime();
    unsigned int getAuthKeyGenNonceLen();
    List(DigestTypes) getDigestLst();
    enum DigestTypes getDigest();
#endif

    void setDefaults();
    std::string getScriptName() { return ScriptName; }
    void setScriptName(std::string scriptFile) { ScriptName = scriptFile; }

    // Client List check
    void InClientClass(SPtr<TSrvMsg> msg);

    // used to be private, but we need access in tests
protected:
    TSrvCfgMgr(const std::string& cfgFile, const std::string& xmlFile);
    static TSrvCfgMgr * Instance;
    static int NextRelayID;
    std::string XmlFile;
    bool reconfigure;

    bool IsDone;
    bool validateConfig();
    bool validateIface(SPtr<TSrvCfgIface> ptrIface);
    bool validateClass(SPtr<TSrvCfgIface> ptrIface, SPtr<TSrvCfgAddrClass> ptrClass);
    List(TSrvCfgIface) SrvCfgIfaceLst;
    List(TSrvCfgIface) InactiveLst;
    List(TSrvCfgClientClass) ClientClassLst;
    bool matchParsedSystemInterfaces(SrvParser *parser);

    // global options
    bool Stateless;
    bool InactiveMode;
    bool GuessMode;
    int  CacheSize;
    ESrvIfaceIdOrder InterfaceIDOrder;
    std::string ScriptName;

#ifndef MOD_DISABLE_AUTH
    unsigned int AuthLifetime;
    unsigned int AuthKeyGenNonceLen;
    List(DigestTypes) DigestLst;
#endif

    // DDNS address
    SPtr<TIPv6Addr> FqdnDdnsAddress;

    // lease-query parameters
    bool BulkLQAccept;
    unsigned short BulkLQTcpPort;
    unsigned int BulkLQMaxConns;
    unsigned int BulkLQTimeout;
};

#endif /* SRVCONFMGR_H */
