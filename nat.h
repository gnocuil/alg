#pragma once
#include "packet.h"
#include "state.h"
#include "ip.h"

class NAT {
public:
//    NAT(StateManager &sm) : sm_(sm) {};
    
    int translate(PacketPtr pkt);
    int begin(PacketPtr pkt);
    void doDPort(PacketPtr pkt);
    void doSPort(PacketPtr pkt);
    void doIP(PacketPtr pkt);
    void finish(PacketPtr pkt);
    void doApp(PacketPtr pkt);

private:
//    StateManager &sm_;
    IP4Port ip4p_;
    IP6Port ip6p_;
    
    int modify(PacketPtr pkt, std::vector<Operation>& ops);
};
extern NAT nat;
