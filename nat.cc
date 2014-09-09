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
        ip4p_ = sm.doMapping(IP6Port(IPv6Addr(ip6->ip6_src), tcp->source), IPv6Addr(ip6->ip6_dst))->ip4p;
    } else {
        iphdr* ip = pkt->getIPHeader();
        ip6p_ = sm.getMapping(IP4Port(IPv4Addr(ip->daddr), tcp->dest), IPv4Addr(ip->saddr))->ip6p;
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
        IPv4Addr daddr = sm.getServerAddr(IPv6Addr(ip6->ip6_dst));
        make_ip4pkt(pkt->obuf_, pkt->getTransportLen(), ip4p_.getIP(), daddr);
    } else {
        iphdr* ip = pkt->getIPHeader();
        IPv6Addr saddr = sm.getServerAddr(IPv4Addr(ip->saddr));
        make_ip6pkt(pkt->obuf_, pkt->getTransportLen(), saddr, ip6p_.getIP());
    }
}

void NAT::finish(PacketPtr pkt)
{
    pkt->updateChecksum();
}

void NAT::doApp(PacketPtr pkt)
{
    DEST dest = pkt->getDEST();
    
	tcphdr* tcp = pkt->getTCPHeader();
	int len = pkt->getTransportLen();
	int hl = pkt->getTCPHeaderLen();
	
	FlowPtr flow = pkt->getFlow();
	if (!flow) return;
	
	int offsetc2s = flow->getOffset(SERVER);
	int offsets2c = flow->getOffset(CLIENT);
    if (dest == SERVER) {
        tcp->th_seq = ntohl(ntohl(tcp->th_seq) + offsetc2s);
	    tcp->th_ack = ntohl(ntohl(tcp->th_ack) - offsets2c);
	} else {
	    tcp->th_ack = ntohl(ntohl(tcp->th_ack) - offsetc2s);
        tcp->th_seq = ntohl(ntohl(tcp->th_seq) + offsets2c);
	}

	if (len > hl) {
	    string str((char*)tcp + hl, (char*)tcp + len);
	    ParserPtr parser = flow->getParser("http", dest);
	    if (parser) {
	        cout << "found parser! " << flow << endl;
	        parser->process(str);
	    }
/*
	    if (str.size() > 5 && str.substr(0, 4) == "EPRT") {
    	    cout << "str:[" << str << "]" << endl;
	        int p;
	        int port;
	        int cnt = 0;
	        char buf1[100] = {0};
	        for (int i = 0; i < str.size(); ++i) {
	            if (str[i] == '|') {
	                str[i] = ' ';
	                ++cnt;
	            }
//	            if (cnt == 2) str[i] = ' ';
	        }
	        sscanf(str.c_str(), "EPRT %d %s %d", &p, buf1, &port);
	        printf("addr=%s p=%d port=%d\n", buf1, p, port);
	        
	        IP4Port ip4p = sm.doMapping(IP6Port(IPv6Addr(buf1), ntohs(port)), IPv6Addr(pkt->getIP6Header()->ip6_dst))->ip4p;
	        
	        static char buf[2000] = {0};
	        sprintf(buf, "EPRT |1|%s|%d|\r\n", ip4p.getIP().getString().c_str(), ntohs(ip4p.getPort()));
	        int newlen = strlen(buf);
	        memcpy((char*)tcp + hl, buf, newlen);
	        newlen += hl;
	        flow->addOffset(SERVER, newlen - len);
	        printf("newlen=%d buf=%s offset=%d\n", newlen, buf, flow->getOffset());
	        pkt->setTransportLen(newlen);
	    }
*/
	}
}

