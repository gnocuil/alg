#pragma once

#include <map>
#include <queue>
#include <boost/shared_ptr.hpp>
#include "ip.h"
#include "parser.h"

const uint16_t MIN_PORT = 21000;
const uint16_t MAX_PORT = 60000;

class Flow {
public:
    Flow() : offset_c2s(0), offset_s2c(0), ignore_(false) {}
    Flow(const IP6Port& ip6p_, const IP4Port& ip4p_, const IPv6Addr& ip6srv_, const IPv4Addr& ip4srv_)
        : ip6p(ip6p_),
          ip4p(ip4p_),
          ip6srv(ip6srv_),
          ip4srv(ip4srv_),
          offset_c2s(0),
          offset_s2c(0),
          ignore_(false),
          protocol("") {}
          
    ~Flow() {std::cout << "Delete " << *this << std::endl;}
    friend std::ostream& operator<< (std::ostream& os, const Flow &f);
    int getOffset(DEST dest);
    void addOffset(DEST dest, int delta);
    
    ParserPtr getParser(std::string protocol, DEST dest);
    
    void setIgnore() {ignore_ = true;}
    bool ignored() {return ignore_;}
    
    void setProtocol(std::string protocol_) {protocol = protocol_;}
    std::string getProtocol() const {return protocol;}
    
    void save(std::string content);
    
//private:
    IP6Port ip6p;
    IPv6Addr ip6srv;
    IP4Port ip4p;
    IPv4Addr ip4srv;
private:
    int offset_c2s;
    int offset_s2c;
    ParserPtr https2c;
    bool ignore_;
    std::string protocol;
    std::map<std::string, ParserPtr> parsers_c2s;
    std::map<std::string, ParserPtr> parsers_s2c;
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
    FlowPtr doMapping(const IP6Port& ip6Port) {return doMapping(ip6Port, ip6srv_cur_);}
    FlowPtr getMapping(const IP6Port& ip6Port, const IPv6Addr& ip6srv);
    FlowPtr getMapping(const IP4Port& ip4Port, const IPv4Addr& ip4srv);
    
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
    
private:
    std::queue<IP4Port> pool_;
    std::map<uint32_t, bool> map_pool_;
    std::map<std::pair<IP6Port, IPv6Addr>, FlowPtr> map64_;
    std::map<std::pair<IP4Port, IPv4Addr>, FlowPtr> map46_;
    IPv6Addr prefix_;
    IPv6Addr ip6srv_cur_;
    
public:
    std::map<std::string, Protocol> protocols;
};
extern StateManager sm;

