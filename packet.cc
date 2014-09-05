#include <cstring>
#include <netinet/in.h>
#include <net/ethernet.h>

#include "packet.h"

static uint16_t tcp_checksum(const void *tcphead, size_t tcplen, const void* saddr, const void*  daddr, int addrlen)
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
    sum += htons(IPPROTO_TCP);
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
    handleTransportLayer(ip_len_, protocol);
}

void Packet::handleTransportLayer(int offset, int protocol)
{
    transport_len_ = obuf_len_ - offset;
    switch (protocol) {
    case IPPROTO_TCP:
        tcp_ = (tcphdr*)(obuf_ + offset);
    default:
        //TODO
        ;
    };
}

void Packet::setTransportLen(int len)
{
    transport_len_ = len;
    obuf_len_ = ip_len_ + transport_len_;
}

void Packet::updateChecksum()
{
    if (tcp_) tcp_->check = 0;
    if (ipVersionBefore_ == 6) {
        if (tcp_) tcp_->check = tcp_checksum(tcp_, transport_len_, &(ip_->saddr), &(ip_->daddr), 4);
        ip_->check = ip_checksum(ip_, transport_len_ + 20);
    } else {
        if (tcp_) tcp_->check = tcp_checksum(tcp_, transport_len_, &(ip6_->ip6_src), &(ip6_->ip6_dst), 16);
    }
}

void Packet::print()
{
    for (int i = 0; i < ibuf_len_; ++i) {
        printf("%02x ", ibuf_[i] & 0xff);
        if (i % 32 == 31) printf("\n");
    }
    if (tcp_) {
        int hl = getTCPHeaderLen();
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

