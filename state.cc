#include <iostream>
#include <cstdio>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "state.h"

using namespace std;

StateManager sm;

void StateManager::init(const std::string file)
{
    analysisMode = false;
    flow_cnt_ = 0;
    using boost::property_tree::ptree;
    ptree pt;
    ifstream fin(file);
    if (!fin) {
        cerr << "Failed reading configuration file: " << file << endl;
        exit(0);
    }
    read_json(fin, pt);
    fin.close();
    
    string prefix = pt.get<std::string>("IPv6Prefix");
    sm.setIPv6Prefix(IPv6Addr(prefix));
    string ip4 = pt.get<std::string>("IPv4Pool");
    sm.addIPv4Pool(IPv4Addr(ip4));

    BOOST_FOREACH(ptree::value_type &v, pt.get_child("Protocols")) {
        string protocolName = v.second.data();
        Protocol protocol;
        try {
            BOOST_FOREACH(ptree::value_type &w, pt.get_child(protocolName)) {
                if (string(w.first.data()) == "ServerToClient") {
                    if (string(w.second.data()) == "stateful") {
                        protocol.ptype_s2c = STATEFUL;
                    } else if (string(w.second.data()) == "none") {
                        protocol.ptype_s2c = NONE;
                    }
                }
                if (string(w.first.data()) == "ClientToServer") {
                    if (string(w.second.data()) == "stateful") {
                        protocol.ptype_c2s = STATEFUL;
                    } else if (string(w.second.data()) == "none") {
                        protocol.ptype_c2s = NONE;
                    }
                }
                if (string(w.first.data()) == "Protocol") {
                    protocol.protocol = string(w.second.data());
                }
            }
        } catch (const exception& ex) {
        }
        std::cout << "add Protocol: " << protocolName << " " << protocol.protocol << std::endl;
        protocols[protocolName] = protocol;
    }
    
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
        //std::cout << "Create Mapping! " << *flow << std::endl;
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

FlowPtr StateManager::doMapping44(const IP4Port& ip4Port1, const IP4Port& ip4Port2)
{//ip4Port1: dest; ip4Port2: src
    pair<IP4Port, IP4Port> key1(ip4Port1, ip4Port2);
    pair<IP4Port, IP4Port> key2(ip4Port2, ip4Port1);
    if (map44_.count(key1) != 0) {
        return map44_[key1];
    }
    if (map44_.count(key2) != 0) {
        return map44_[key2];
    }
    if (ntohs(ip4Port2.getPort()) == 80 && ntohs(ip4Port1.getPort()) != 80) {
        key1 = key2;
    }
    //printf("doMapping: destPort=%d srcPort=%d\n", key1.first.getPort(), key1.second.getPort());
    ++flow_cnt_;
        
    FlowPtr flow = FlowPtr(new Flow(key1.first, key1.second));
    map44_[key1] = flow;
    return flow;
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

