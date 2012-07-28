/*
 * Dibbler - a portable DHCPv6
 *
 * authors: Tomasz Mrugalski <thomson@klub.com.pl>
 *          Marek Senderski <msend@o2.pl>
 *
 * released under GNU GPL v2 only licence
 *
 */

#include "ClntParsGlobalOpt.h"
#include "Portable.h"
#include "DHCPConst.h"
#include "Logger.h"

using namespace std;

TClntParsGlobalOpt::TClntParsGlobalOpt()
    :TClntParsIfaceOpt() {
    this->WorkDir        = WORKDIR;
    this->PrefixLength   = CLIENT_DEFAULT_PREFIX_LENGTH;
    this->Digest         = CLIENT_DEFAULT_DIGEST;
    this->AnonInfRequest = false;
    this->InactiveMode   = false;
    this->InsistMode     = false;
    this->FQDNFlagS      = CLIENT_DEFAULT_FQDN_FLAG_S;
    this->Experimental   = false;
    this->UseConfirm     = true;

    this->AuthEnabled    = false;
    this->AuthAcceptMethods.clear();
}

TClntParsGlobalOpt::~TClntParsGlobalOpt() {
}

void TClntParsGlobalOpt::setWorkDir(const std::string& dir) {
    WorkDir = dir;
}

string TClntParsGlobalOpt::getWorkDir() {
    return WorkDir;
}

void TClntParsGlobalOpt::setOnLinkPrefixLength(int len) {
    PrefixLength = len;
}

int TClntParsGlobalOpt::getOnLinkPrefixLength() {
    return PrefixLength;
}

void TClntParsGlobalOpt::setDigest(DigestTypes digest) {
    this->Digest = digest;
}

DigestTypes TClntParsGlobalOpt::getDigest() {
    return this->Digest;
}

void TClntParsGlobalOpt::setAnonInfRequest(bool anonymous) {
    this->AnonInfRequest = anonymous;
}

bool TClntParsGlobalOpt::getAnonInfRequest() {
    return this->AnonInfRequest;
}

void TClntParsGlobalOpt::setInsistMode(bool insist)
{
    InsistMode = insist;
}

bool TClntParsGlobalOpt::getInsistMode()
{
    return InsistMode;
}

void TClntParsGlobalOpt::setInactiveMode(bool flex)
{
    InactiveMode = flex;
}

bool TClntParsGlobalOpt::getInactiveMode()
{
    return InactiveMode;
}

void TClntParsGlobalOpt::setExperimental()
{
    Experimental = true;
}

bool TClntParsGlobalOpt::getExperimental()
{
    return Experimental;
}

void TClntParsGlobalOpt::setAuthAcceptMethods(List(DigestTypes) lst)
{
    AuthAcceptMethods = lst;
    Log(Debug) << "AUTH: " << lst.count() << " method(s) accepted." << LogEnd;
}

List(DigestTypes) TClntParsGlobalOpt::getAuthAcceptMethods()
{
    return AuthAcceptMethods;
}

void TClntParsGlobalOpt::setFQDNFlagS(bool s)
{
    FQDNFlagS = s;
}

bool TClntParsGlobalOpt::getFQDNFlagS()
{
    return FQDNFlagS;
}

void TClntParsGlobalOpt::setAuthEnabled(bool enabled)
{
    AuthEnabled = enabled;
    Log(Debug) << "AUTH: Authentication " << (enabled?"enabled":"disabled")
               << "." << LogEnd;
}

bool TClntParsGlobalOpt::getAuthEnabled()
{
    return AuthEnabled;
}

void TClntParsGlobalOpt::setConfirm(bool conf)
{
    UseConfirm = conf;
}

bool TClntParsGlobalOpt::getConfirm()
{
    return UseConfirm;
}
