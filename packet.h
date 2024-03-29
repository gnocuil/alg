#pragma once

#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <cstdio>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include "ip.h"
#include "state.h"
#include <cstring>

const int BUF_LEN = 65536;

class Packet {
public:
    Packet()
        : tcp_(NULL),
          udp_(NULL)
    {
    }
//    void setIbufLen(int len) { ibuf_len_ = len; }
    void setIbuf(uint8_t* ibuf, int len) { memcpy(ibuf_, ibuf, len); ibuf_len_=len; }
//    uint8_t** getIbuf() { return (uint8_t**)(&ibuf_); }
    void unpack();
    int getObufLen() const { return obuf_len_; }
    
    iphdr* getIPHeader() const { return ip_; }
    ip6_hdr* getIP6Header() const { return ip6_; }
    bool isTCP() const { return tcp_ ? true : false; }
    bool isUDP() const { return udp_ ? true : false; }
    int getTransportHeaderLen() const;
    tcphdr* getTCPHeader() const { return tcp_; }
    tcphdr* getIbufTCPHeader() const { return tcp_old_; }
    udphdr* getUDPHeader() const { return udp_; }
    udphdr* getIbufUDPHeader() const { return udp_old_; }
    int getTransportLen() const { return transport_len_; }
    int getIPVersion() const { return ipVersionBefore_ == 4 ? 6 : 4; }
    
    void setTransportLen(int len);
    void updateChecksum();
    
    IPv4Addr getDest4() const { return IPv4Addr(ip_->daddr); }
    IPv6Addr getDest6() const { return IPv6Addr(ip6_->ip6_dst); }
    
    DEST getDEST() const { return ipVersionBefore_ == 4 ? CLIENT : SERVER; }
    FlowPtr getFlow();
    
    u_int16_t getSourcePort() const;
    u_int16_t getDestPort() const;
    
    void print();
    
    uint32_t nextSeq() { return nxt_seq_; }//get next tcp seq (before modification)
    uint32_t curSeq() { return cur_seq_; }//get current tcp seq (before modification)
    
    u_int16_t hw_protocol;
    u_int32_t id;
    
//private:
    uint8_t ibuf_[BUF_LEN];
    int ibuf_len_;
    
    uint8_t obuf_[BUF_LEN];
    uint32_t port_t;
    
private:
    int obuf_len_;
    int transport_len_;
    int ip_len_;
    int ipVersionBefore_;
//    bool istcp_;
    
    iphdr* ip_;
    ip6_hdr* ip6_;
    tcphdr* tcp_;
    tcphdr* tcp_old_;
    
    udphdr* udp_;
    udphdr* udp_old_;
    
    uint32_t nxt_seq_;
    uint32_t cur_seq_;
    
    void handleTransportLayer(int offset, int offset_old, int protocol);
    
};

typedef boost::shared_ptr<Packet> PacketPtr;
