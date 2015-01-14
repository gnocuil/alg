#pragma once
#include "packet.h"
#include "state.h"
#include "ip.h"
#include "socket.h"

class NAT {
public:
    NAT(bool init = true) {
        if (init) {
            socket4.init();
            socket6.init();
        }
    }
    
    int translate(PacketPtr pkt);
    int begin(PacketPtr pkt);
    void doDPort(PacketPtr pkt);
    void doSPort(PacketPtr pkt);
    void doIP(PacketPtr pkt);
    void finish(PacketPtr pkt);
    void doApp(PacketPtr pkt);
    
    bool doExtraTCP(PacketPtr pkt);
    
    int tid;

private:
//    StateManager &sm_;
    IP4Port ip4p_;
    IP6Port ip6p_;
    
    FlowPtr exttcpflow_;
    bool modified_;
    SocketRaw4 socket4;
    SocketRaw6 socket6;
};
//extern NAT nat;
