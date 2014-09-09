#pragma once

#include <map>
#include <queue>
#include <boost/shared_ptr.hpp>
#include "ip.h"
#include "parser.h"

const uint16_t MIN_PORT = 20000;
const uint16_t MAX_PORT = 60000;

class Flow {
public:
    Flow() : offset_c2s(0), offset_s2c(0) {}
    Flow(const IP6Port& ip6p_, const IP4Port& ip4p_, const IPv6Addr& ip6srv_, const IPv4Addr& ip4srv_)
        : ip6p(ip6p_),
          ip4p(ip4p_),
          ip6srv(ip6srv_),
          ip4srv(ip4srv_),
          offset_c2s(0),
          offset_s2c(0) {}
    friend std::ostream& operator<< (std::ostream& os, const Flow &f);
    int getOffset(DEST dest);
    void addOffset(DEST dest, int delta);
    
    ParserPtr getParser(std::string protocol, DEST dest);
    
//private:
    IP6Port ip6p;
    IPv6Addr ip6srv;
    IP4Port ip4p;
    IPv4Addr ip4srv;
private:
    int offset_c2s;
    int offset_s2c;
    ParserPtr https2c;
};

typedef boost::shared_ptr<Flow> FlowPtr;
std::ostream& operator<< (std::ostream& os, const FlowPtr &f);

class StateManager {
public:
    void addIPv4Pool(const IPv4Addr& ip);
    
    void setIPv6Prefix(const IPv6Addr& prefix);
    IPv6Addr getServerAddr(const IPv4Addr& ip);
    IPv4Addr getServerAddr(const IPv6Addr& ip);
    
    bool isAddrInRange(const IPv6Addr& ip);
    bool isAddrInRange(const IPv4Addr& ip);
    
    FlowPtr doMapping(const IP6Port& ip6Port, const IPv6Addr& ip6srv);
    FlowPtr getMapping(const IP6Port& ip6Port, const IPv6Addr& ip6srv);
    FlowPtr getMapping(const IP4Port& ip4Port, const IPv4Addr& ip4srv);
    
private:
    std::queue<IP4Port> pool_;
    std::map<uint32_t, bool> map_pool_;
    std::map<std::pair<IP6Port, IPv6Addr>, FlowPtr> map64_;
    std::map<std::pair<IP4Port, IPv4Addr>, FlowPtr> map46_;
    IPv6Addr prefix_;
};
extern StateManager sm;

