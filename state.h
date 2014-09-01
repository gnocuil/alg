#pragma once

#include <map>
#include <queue>
#include "ip.h"

class State {
public:
    void addIPv4Pool(IPv4Addr ip);
    
    IP4Port doMapping(IP6Port ip6Port);
    IP6Port getMapping(IP4Port ip6Port);
    
private:
    std::queue<IP4Port> pool_;
    std::map<IP6Port, IP4Port> map64_;
    std::map<IP4Port, IP6Port> map46_;
};
