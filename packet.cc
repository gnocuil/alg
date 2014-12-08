#include <cstring>
#include <netinet/in.h>
#include <net/ethernet.h>

#include "packet.h"

using namespace std;

static uint16_t tcp_checksum(const void *tcphead, size_t tcplen, const void* saddr, const void*  daddr, int addrlen, uint8_t protocol)
{
    const uint16_t *buf = (uint16_t*)tcphead;
    uint16_t *ip_src = (uint16_t*)saddr, *ip_dst = (uint16_t*)daddr;
    uint32_t sum = 0;
    size_t len = tcplen;

    // Calculate the sum                                            //
    while (len > 1) {
        sum += *buf++;
        if (sum & 0x80000000)
            sum = (sum & 0xFFFF) + (sum >> 16);
        len -= 2;
    }
    
    if ( len & 1 )
        // Add the padding if the packet lenght is odd          //
        sum += *((uint8_t *)buf);

    // Add the pseudo-header                                        //
    while (addrlen) {
        addrlen -= 2;
        sum += *(ip_src++);
        sum += *(ip_dst++);
    }
    sum += htons(protocol);
    sum += htons(tcplen);

    // Add the carries                                              //
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    // Return the one's complement of sum                           //
    return ( (uint16_t)(~sum)  );
}

static uint16_t ip_checksum(const void *iphead, size_t iplen)
{
    const uint16_t *buf = (uint16_t*)iphead;
    uint32_t sum = 0;
    size_t len = iplen;

    // Calculate the sum                                            //
    while (len > 1) {
        sum += *buf++;
        if (sum & 0x80000000)
            sum = (sum & 0xFFFF) + (sum >> 16);
        len -= 2;
    }
    
    if ( len & 1 )
        // Add the padding if the packet lenght is odd          //
        sum += *((uint8_t *)buf);

    // Add the carries                                              //
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    // Return the one's complement of sum                           //
    return ( (uint16_t)(~sum)  );
}

void Packet::unpack()
{
    int ip_len_old;
    int protocol;
    if (hw_protocol == ETHERTYPE_IPV6) {
        ipVersionBefore_ = 6;
        ip6_ = (ip6_hdr*)ibuf_;
        protocol = ip6_->ip6_nxt;
        ip_len_old = 40;//TODO: handle extension headers
        ip_len_ = 20;
        ip_ = (iphdr*)obuf_;
    } else if (hw_protocol == ETHERTYPE_IP) {
        ipVersionBefore_ = 4;
        ip_ = (iphdr*)ibuf_;
        protocol = ip_->protocol;
        ip_len_old = 20;//TODO: handle other length
        ip_len_ = 40;
        ip6_ = (ip6_hdr*)obuf_;
    } else {
        //TODO: ignore the packet
        return;
    }
    memcpy(obuf_ + ip_len_, ibuf_ + ip_len_old, ibuf_len_ - ip_len_old);
    obuf_len_ = ibuf_len_ - ip_len_old + ip_len_;
    handleTransportLayer(ip_len_, ip_len_old, protocol);
}

void Packet::handleTransportLayer(int offset, int offset_old, int protocol)
{
    transport_len_ = obuf_len_ - offset;
    switch (protocol) {
    case IPPROTO_TCP:
        tcp_ = (tcphdr*)(obuf_ + offset);
        tcp_old_ = (tcphdr*)(ibuf_ + offset_old);
        break;
    case IPPROTO_UDP:
        udp_ = (udphdr*)(obuf_ + offset);
        udp_old_ = (udphdr*)(ibuf_ + offset_old);
        break;
    default:
        //TODO
        ;
    };
}

void Packet::setTransportLen(int len)
{
    transport_len_ = len;
    obuf_len_ = ip_len_ + transport_len_;
    if (udp_) {
        udp_->len = ntohs((u_int16_t)len);
    }
}

void Packet::updateChecksum()
{
    if (tcp_) tcp_->check = 0;
    if (udp_) udp_->check = 0;
    if (ipVersionBefore_ == 6) {
        if (tcp_) tcp_->check = tcp_checksum(tcp_, transport_len_, &(ip_->saddr), &(ip_->daddr), 4, IPPROTO_TCP);
        if (udp_) udp_->check = tcp_checksum(udp_, transport_len_, &(ip_->saddr), &(ip_->daddr), 4, IPPROTO_UDP);
        ip_->check = ip_checksum(ip_, transport_len_ + 20);
    } else {
        if (tcp_) tcp_->check = tcp_checksum(tcp_, transport_len_, &(ip6_->ip6_src), &(ip6_->ip6_dst), 16, IPPROTO_TCP);
        if (udp_) udp_->check = tcp_checksum(udp_, transport_len_, &(ip6_->ip6_src), &(ip6_->ip6_dst), 16, IPPROTO_UDP);
    }
}

FlowPtr Packet::getFlow() {
    FlowPtr ret = FlowPtr();
    if (tcp_) {
        if (ipVersionBefore_ == 6) {
            ret = sm.getMapping(IP6Port(IPv6Addr(ip6_->ip6_src), tcp_->source), IPv6Addr(ip6_->ip6_dst));
        } else if (sm.analysisMode) {
            ret = sm.doMapping44(IP4Port(IPv4Addr(ip_->daddr), tcp_->dest), IP4Port(IPv4Addr(ip_->saddr), tcp_->source));
        } else {
            ret = sm.getMapping(IP4Port(IPv4Addr(ip_->daddr), tcp_->dest), IPv4Addr(ip_->saddr));
        }
    } else if (udp_) {
        if (ipVersionBefore_ == 6) {
            ret = sm.getMapping(IP6Port(IPv6Addr(ip6_->ip6_src), udp_->source), IPv6Addr(ip6_->ip6_dst));
        } else if (sm.analysisMode) {
            ret = sm.doMapping44(IP4Port(IPv4Addr(ip_->daddr), udp_->dest), IP4Port(IPv4Addr(ip_->saddr), udp_->source));
        } else {
            ret = sm.getMapping(IP4Port(IPv4Addr(ip_->daddr), udp_->dest), IPv4Addr(ip_->saddr));
        }
    }
    return ret;
}

u_int16_t Packet::getSourcePort() const {
    if (tcp_)
        return tcp_->source;
    if (udp_)
        return udp_->source;
    return 0;//TODO: throw?
}

u_int16_t Packet::getDestPort() const {
    if (tcp_)
        return tcp_->dest;
    if (udp_)
        return udp_->dest;
    return 0;//TODO: throw?
}

int Packet::getTransportHeaderLen() const {
    if (tcp_)
        return tcp_->th_off * 4;
    if (udp_)
        return 8;
    return 0;
}

void Packet::print()
{
    /*
    for (int i = 0; i < ibuf_len_; ++i) {
        printf("%02x ", ibuf_[i] & 0xff);
        if (i % 32 == 31) printf("\n");
    }
    */
    if (tcp_) {
        int hl = getTransportHeaderLen();
        int tl = getTransportLen();
        if (hl < tl) {
            putchar('\n');
            printf("hl=%d tl=%d\n--------START------------\n", hl, tl);
            for (int i = hl; i < tl; ++i)
                putchar(*((char*)tcp_ + i));
            printf("---------END-----------\n");
        }
    }
    printf("\n");
}

