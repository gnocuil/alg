#include <cstring>
#include <iostream>
#include "nat.h"
#include "ip.h"

using namespace std;

static void make_ip4pkt(u_int8_t* header, u_int16_t payload_len, const IPv4Addr& ip4saddr, const IPv4Addr& ip4daddr)
{
    memset(header, 0, 20);
    iphdr* ip = (iphdr*)header;
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = ntohs(payload_len);
    ip->id = 0;
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = 0x06;
    ip->check = 0;
    
	ip->daddr = ip4daddr.getInt();
    ip->saddr = ip4saddr.getInt();
}

static void make_ip6pkt(u_int8_t* header, u_int16_t payload_len, const IPv6Addr& ip6saddr, const IPv6Addr& ip6daddr)
{
    memset(header, 0, 20);
    
    ip6_hdr* ip6 = (ip6_hdr*)header;
    ip6->ip6_flow = 0x60;
    ip6->ip6_plen = ntohs(payload_len);
    ip6->ip6_hops = 64;
    ip6->ip6_nxt = 0x06;
    
    ip6->ip6_src = ip6saddr.getIn6Addr();
    ip6->ip6_dst = ip6daddr.getIn6Addr();
}

void NAT::begin(PacketPtr pkt)
{
    tcphdr* tcp = pkt->getTCPHeader();//TODO: not tcp
    if (pkt->getIPVersion() == 4) {
        ip6_hdr* ip6 = pkt->getIP6Header();
        ip4p_ = sm_.doMapping(IP6Port(IPv6Addr(ip6->ip6_src), tcp->source));
    } else {
        iphdr* ip = pkt->getIPHeader();
        ip6p_ = sm_.getMapping(IP4Port(IPv4Addr(ip->daddr), tcp->dest));
    }
}

void NAT::doSPort(PacketPtr pkt)
{
    tcphdr* tcp = pkt->getTCPHeader();//TODO: not tcp
    if (pkt->getIPVersion() == 4) {
        tcp->source = ip4p_.getPort();
    } else {
        //do nothing
    }
}

void NAT::doDPort(PacketPtr pkt)
{
    tcphdr* tcp = pkt->getTCPHeader();//TODO: not tcp
    if (pkt->getIPVersion() == 6) {
        tcp->dest = ip6p_.getPort();
    } else {
        //do nothing
    }
}

void NAT::doIP(PacketPtr pkt)
{
    if (pkt->getIPVersion() == 4) {
        ip6_hdr* ip6 = pkt->getIP6Header();
        IPv4Addr daddr = sm_.getServerAddr(IPv6Addr(ip6->ip6_dst));
        make_ip4pkt(pkt->obuf_, pkt->getTransportLen(), ip4p_.getIP(), daddr);
    } else {
        iphdr* ip = pkt->getIPHeader();
        IPv6Addr saddr = sm_.getServerAddr(IPv4Addr(ip->saddr));
        make_ip6pkt(pkt->obuf_, pkt->getTransportLen(), saddr, ip6p_.getIP());
    }
}

void NAT::finish(PacketPtr pkt)
{
    pkt->updateChecksum();
}

