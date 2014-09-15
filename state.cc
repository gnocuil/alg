#include <iostream>
#include <cstdio>
#include "state.h"

using namespace std;

StateManager sm;

std::ostream& operator<< (std::ostream& os, const Flow &f)
{
    return os << "Flow(" << f.ip6p << "<->" << f.ip6srv << " <=====> " << f.ip4p << "<->" << f.ip4srv << ")";
}

std::ostream& operator<< (std::ostream& os, const FlowPtr &f)
{
    if (!f) return os << "Flow(NULL)";
    return os << *f;
}

ParserPtr Flow::getParser(std::string protocol, DEST dest)
{
    if (protocol == "http") {
        if (dest == CLIENT) {
            if (!https2c) {
                https2c = Parser::make(protocol);
            }
            return https2c;
        }
    } else if (protocol == "ftp") {
        if (dest == SERVER) {
            return Parser::make(protocol);
        }
    }
    return ParserPtr();
}

int Flow::getOffset(DEST dest)
{
    if (dest == SERVER)
        return offset_c2s;
    else
        return offset_s2c;
}

void Flow::addOffset(DEST dest, int delta)
{
    if (dest == SERVER)
        offset_c2s += delta;
    else
        offset_s2c += delta;
}

void StateManager::addIPv4Pool(const IPv4Addr& ip)
{
    for (uint16_t p = MIN_PORT; p <= MAX_PORT; ++p) {
        pool_.push(IP4Port(ip, htons(p)));
    }
    map_pool_[ip.getInt()] = true;
}
    
FlowPtr StateManager::doMapping(const IP6Port& ip6Port, const IPv6Addr& ip6srv)
{
    pair<IP6Port, IPv6Addr> key(ip6Port, ip6srv);
    if (map64_.count(key) != 0) {
        return map64_[key];
    } else if (pool_.size() > 0) {
        IP4Port ip4Port = pool_.front();
        pool_.pop();
        FlowPtr flow = FlowPtr(new Flow(ip6Port, ip4Port, ip6srv, getServerAddr(ip6srv)));
        pair<IP4Port, IPv4Addr> newkey(ip4Port, getServerAddr(ip6srv));
        map64_[key] = flow;
        map46_[newkey] = flow;
        std::cout << "Create Mapping! " << *flow << std::endl;
        return flow;
    } else {
        return FlowPtr();
    }
}

FlowPtr StateManager::getMapping(const IP6Port& ip6Port, const IPv6Addr& ip6srv)
{
    pair<IP6Port, IPv6Addr> key(ip6Port, ip6srv);
    if (map64_.count(key) != 0) {
        return map64_[key];
    } else {
        return FlowPtr();
    }
}

FlowPtr StateManager::getMapping(const IP4Port& ip4Port, const IPv4Addr& ip4srv)
{
    pair<IP4Port, IPv4Addr> key(ip4Port, ip4srv);
    if (map46_.count(key) != 0) {
        return map46_[key];
    } else {
        return FlowPtr();
    }
}

void StateManager::setIPv6Prefix(const IPv6Addr& prefix)
{
    prefix_ = prefix;
}

IPv6Addr StateManager::getServerAddr(const IPv4Addr& ip)
{
    in6_addr addr = prefix_.getIn6Addr();
    addr.s6_addr32[3] = ip.getInt();
    return IPv6Addr(addr);
}

IPv4Addr StateManager::getServerAddr(const IPv6Addr& ip)
{
    in6_addr addr = ip.getIn6Addr();
    return IPv4Addr(addr.s6_addr32[3]);
}

bool StateManager::isAddrInRange(const IPv6Addr& ip)
{
    in6_addr addr1 = prefix_.getIn6Addr();
    in6_addr addr2 = ip.getIn6Addr();
    return (addr1.s6_addr32[0] == addr2.s6_addr32[0]) &&
           (addr1.s6_addr32[1] == addr2.s6_addr32[1]) &&
           (addr1.s6_addr32[2] == addr2.s6_addr32[2]);
}

bool StateManager::isAddrInRange(const IPv4Addr& ip)
{
    return map_pool_[ip.getInt()];
}


