#pragma once

#include <map>
#include <queue>
#include <boost/shared_ptr.hpp>
#include "ip.h"
#include "flow.h"

const uint16_t MIN_PORT = 21000;
const uint16_t MAX_PORT = 60000;

class StateManager {
public:
    void addIPv4Pool(const IPv4Addr& ip);
    
    void setIPv6Prefix(const IPv6Addr& prefix);
    IPv6Addr getServerAddr(const IPv4Addr& ip);
    IPv4Addr getServerAddr(const IPv6Addr& ip);
    
    bool isAddrInRange(const IPv6Addr& ip);
    bool isAddrInRange(const IPv4Addr& ip);
    
    FlowPtr doMapping(const IP6Port& ip6Port, const IPv6Addr& ip6srv);
    FlowPtr doMapping(const IP6Port& ip6Port) {return doMapping(ip6Port, ip6srv_cur_);}
    FlowPtr getMapping(const IP6Port& ip6Port, const IPv6Addr& ip6srv);
    FlowPtr getMapping(const IP4Port& ip4Port, const IPv4Addr& ip4srv);
    
    //do & get mapping for 4-4 (analysis mode only)
    FlowPtr doMapping44(const IP4Port& ip4Port1, const IP4Port& ip4Port2);
    
    void setCurIPv6SrvAddr(const IPv6Addr& addr) {ip6srv_cur_=addr;}
    
    //init configuration from a jason format file
    void init(const std::string file);
    
    enum PROCESS_TYPE {
        STATELESS = 0,
        STATEFUL,
        NONE
    };
    
    class Protocol {
    public:
        Protocol()
          : protocol("tcp"),
            ptype_c2s(STATELESS),
            ptype_s2c(STATELESS)
          {}
        std::string protocol;
        PROCESS_TYPE ptype_c2s;
        PROCESS_TYPE ptype_s2c;
    };
    
    bool analysisMode;
    
private:
    std::queue<IP4Port> pool_;
    std::map<uint32_t, bool> map_pool_;
    std::map<std::pair<IP6Port, IPv6Addr>, FlowPtr> map64_;
    std::map<std::pair<IP4Port, IPv4Addr>, FlowPtr> map46_;
    std::map<std::pair<IP4Port, IP4Port>, FlowPtr> map44_;//for analysis mode
    IPv6Addr prefix_;
    IPv6Addr ip6srv_cur_;
    
public:
    std::map<std::string, Protocol> protocols;
};
extern StateManager sm;

