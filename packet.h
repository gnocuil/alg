#pragma once

#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <cstdio>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include "ip.h"

const int BUF_LEN = 2000;

class Packet {
public:
    Packet()
        : tcp_(NULL)
    {
    }
    void setIbufLen(int len) { ibuf_len_ = len; }
    uint8_t** getIbuf() { return (uint8_t**)(&ibuf_); }
    void unpack();
    int getObufLen() const { return obuf_len_; }
    
    iphdr* getIPHeader() const { return ip_; }
    ip6_hdr* getIP6Header() const { return ip6_; }
    bool isTCP() const { return tcp_ ? true : false; }
    int getTCPHeaderLen() const { return tcp_ ? tcp_->th_off * 4 : 0; }
    tcphdr* getTCPHeader() const { return tcp_; }
    int getTransportLen() const { return transport_len_; }
    int getIPVersion() const { return ipVersionBefore_ == 4 ? 6 : 4; }
    
    void setTransportLen(int len);
    void updateChecksum();
    
    IPv4Addr getDest4() const { return IPv4Addr(ip_->daddr); }
    IPv6Addr getDest6() const { return IPv6Addr(ip6_->ip6_dst); }
    
    void print();
    
    u_int16_t hw_protocol;
    u_int32_t id;
    
//private:
    uint8_t *ibuf_;
    int ibuf_len_;
    
    uint8_t obuf_[BUF_LEN];
    
private:
    int obuf_len_;
    int transport_len_;
    int ip_len_;
    int ipVersionBefore_;
//    bool istcp_;
    
    iphdr* ip_;
    ip6_hdr* ip6_;
    tcphdr* tcp_;
    
    void handleTransportLayer(int offset, int protocol);
    
};

typedef boost::shared_ptr<Packet> PacketPtr;
