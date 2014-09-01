#include <iostream>
#include <cstdio>
#include "state.h"

using namespace std;

void StateManager::addIPv4Pool(const IPv4Addr& ip)
{
    for (uint16_t p = MIN_PORT; p <= MAX_PORT; ++p) {
        pool_.push(IP4Port(ip, htons(p)));
    }
    map_pool_[ip.getInt()] = true;
}
    
IP4Port StateManager::doMapping(const IP6Port& ip6Port)
{
    if (map64_.count(ip6Port) != 0) {
        return map64_[ip6Port];
    } else if (pool_.size() > 0) {
        IP4Port ip4Port = pool_.front();
        pool_.pop();
        map64_[ip6Port] = ip4Port;
        map46_[ip4Port] = ip6Port;
        std::cout << "Create Mapping! " << ip6Port << " <-> " << ip4Port << std::endl;
        return ip4Port;
    } else {
        return IP4Port();
    }
}

IP6Port StateManager::getMapping(const IP4Port& ip4Port)
{
    if (map46_.count(ip4Port) != 0) {
        return map46_[ip4Port];
    } else {
        return IP6Port();
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

