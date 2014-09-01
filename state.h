#pragma once

#include <map>
#include <queue>
#include "ip.h"

const uint16_t MIN_PORT = 20000;
const uint16_t MAX_PORT = 60000;

class StateManager {
public:
    void addIPv4Pool(const IPv4Addr& ip);
    
    void setIPv6Prefix(const IPv6Addr& prefix);
    IPv6Addr getServerAddr(const IPv4Addr& ip);
    IPv4Addr getServerAddr(const IPv6Addr& ip);
    
    bool isAddrInRange(const IPv6Addr& ip);
    bool isAddrInRange(const IPv4Addr& ip);
    
    IP4Port doMapping(const IP6Port& ip6Port);
    IP6Port getMapping(const IP4Port& ip4Port);
    
private:
    std::queue<IP4Port> pool_;
    std::map<uint32_t, bool> map_pool_;
    std::map<IP6Port, IP4Port> map64_;
    std::map<IP4Port, IP6Port> map46_;
    IPv6Addr prefix_;
};
