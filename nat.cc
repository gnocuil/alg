#include <cstring>
#include <iostream>
#include <net/ethernet.h>
#include "socket.h"
#include "nat.h"
#include "ip.h"
#include "state.h"

using namespace std;

NAT nat;

static void make_ip4pkt(u_int8_t* header, u_int16_t payload_len, const IPv4Addr& ip4saddr, const IPv4Addr& ip4daddr, uint8_t protocol)
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
    ip->protocol = protocol;
    ip->check = 0;
    
	ip->daddr = ip4daddr.getInt();
    ip->saddr = ip4saddr.getInt();
}

static void make_ip6pkt(u_int8_t* header, u_int16_t payload_len, const IPv6Addr& ip6saddr, const IPv6Addr& ip6daddr, uint8_t protocol)
{
    memset(header, 0, 20);
    
    ip6_hdr* ip6 = (ip6_hdr*)header;
    ip6->ip6_flow = 0x60;
    ip6->ip6_plen = ntohs(payload_len);
    ip6->ip6_hops = 64;
    ip6->ip6_nxt = protocol;
    
    ip6->ip6_src = ip6saddr.getIn6Addr();
    ip6->ip6_dst = ip6daddr.getIn6Addr();
}

int NAT::translate(PacketPtr pkt)
{
    if (pkt->hw_protocol == ETHERTYPE_IPV6) {
        if (!sm.isAddrInRange(pkt->getDest6()))
            return 0;
    } else {
        if (!sm.isAddrInRange(pkt->getDest4()))
            return 0;
    }
    if (!pkt->isTCP() && !pkt->isUDP()) {
        return 0;
    }
    if (begin(pkt))
        return 0;puts("nat.cc line#58");
    try {
        doApp(pkt);
    } catch (const std::exception& ex) {
        cerr << "Exception in doApp: " << ex.what() << endl;
    } catch (...) {
        cerr << "Exception in doApp: ..." << endl;
    }
    doSPort(pkt);
    doDPort(pkt);
    doIP(pkt);
    finish(pkt);puts("nat.cc line#63");
    if (pkt->hw_protocol == ETHERTYPE_IPV6) {
        socket4.send(pkt->obuf_, pkt->getObufLen(), pkt->getDest4());
    } else {
        socket6.send(pkt->obuf_, pkt->getObufLen(), pkt->getDest6());
    }
    return 1;
}

int NAT::begin(PacketPtr pkt)
{
    //tcphdr* tcp = pkt->getTCPHeader();//TODO: not tcp
    if (pkt->getIPVersion() == 4) {
        ip6_hdr* ip6 = pkt->getIP6Header();
        ip4p_ = sm.doMapping(IP6Port(IPv6Addr(ip6->ip6_src), pkt->getSourcePort()), IPv6Addr(ip6->ip6_dst))->ip4p;
        sm.setCurIPv6SrvAddr(IPv6Addr(ip6->ip6_dst));
    } else {
        iphdr* ip = pkt->getIPHeader();
        FlowPtr f = sm.getMapping(IP4Port(IPv4Addr(ip->daddr), pkt->getDestPort()), IPv4Addr(ip->saddr));
        if (!f) return 1;
        ip6p_ = f->ip6p;
    }
    return 0;
}

void NAT::doSPort(PacketPtr pkt)
{
    tcphdr* tcp = pkt->getTCPHeader();
    udphdr* udp = pkt->getUDPHeader();
    if (pkt->getIPVersion() == 4) {
        if (pkt->isTCP()) tcp->source = ip4p_.getPort();
        if (pkt->isUDP()) udp->source = ip4p_.getPort();
    } else {
        //do nothing
    }
}

void NAT::doDPort(PacketPtr pkt)
{
    tcphdr* tcp = pkt->getTCPHeader();
    udphdr* udp = pkt->getUDPHeader();
    if (pkt->getIPVersion() == 6) {
        if (pkt->isTCP()) tcp->dest = ip6p_.getPort();
        if (pkt->isUDP()) udp->dest = ip6p_.getPort();
    } else {
        //do nothing
    }
}

void NAT::doIP(PacketPtr pkt)
{
    if (pkt->getIPVersion() == 4) {
        ip6_hdr* ip6 = pkt->getIP6Header();
        IPv4Addr daddr = sm.getServerAddr(IPv6Addr(ip6->ip6_dst));
        make_ip4pkt(pkt->obuf_, pkt->getTransportLen(), ip4p_.getIP(), daddr, ip6->ip6_nxt);
    } else {
        iphdr* ip = pkt->getIPHeader();
        IPv6Addr saddr = sm.getServerAddr(IPv4Addr(ip->saddr));
        make_ip6pkt(pkt->obuf_, pkt->getTransportLen(), saddr, ip6p_.getIP(), ip->protocol);
    }
}

void NAT::finish(PacketPtr pkt)
{
    pkt->updateChecksum();
}

int NAT::modify(PacketPtr pkt, std::vector<Operation>& ops, ParserPtr parser)
{
    if (ops.size() == 0)
        return 0;
    sort(ops.begin(), ops.end());
    int len = pkt->getTransportLen();
	int hl = pkt->getTransportHeaderLen();
	len -= hl;
	char *d, *s;
	if (pkt->isTCP()) {
	    d = (char*)pkt->getTCPHeader() + hl;
    	s = (char*)pkt->getIbufTCPHeader() + hl;
    } else if (pkt->isUDP()) {
	    d = (char*)pkt->getUDPHeader() + hl;
    	s = (char*)pkt->getIbufUDPHeader() + hl;
    } else {
        return 0;
    }
	int len_old = len;
	for (int i = 0; i < ops.size(); ++i) {
	    printf("replace : %d %d [", ops[i].start_pos, ops[i].end_pos);
	    for (int j = ops[i].start_pos; j < ops[i].end_pos; ++j)
	        putchar(s[j]);
	    printf("] with <%s>\n", ops[i].newdata.c_str());
	    int delta = ops[i].newdata.size() - (ops[i].end_pos - ops[i].start_pos);
	    printf("delta=%d\n", delta);
	    len += delta;
	}
	printf("newlen=%d\n", len);
	int cnt = 0, pd, ps;
	bool inside = false;
	for (pd = ps = ops[0].start_pos; pd < len; ++pd) {
	    if (!inside && ps == ops[cnt].start_pos) {
	        if (ops[cnt].newdata.size() == 0) {
	            ps = ops[cnt++].end_pos;
	        } else {
    	        inside = true;
	            ps = 0;
	        }
	    }
	    if (inside) {
	        d[pd] = ops[cnt].newdata[ps++];
	        if (ps == ops[cnt].newdata.size()) {
	            inside = false;
	            ps = ops[cnt++].end_pos;
	        }
	    } else {
	        d[pd] = s[ps++];
	    }
	}
	
	int maxPos = parser->maxPos;//check max length
	if (maxPos > 0) {
	    for (int i = 0; i < ops.size(); ++i) {
	        if (ops[i].end_pos <= parser->maxPos) {
        	    int delta = ops[i].newdata.size() - (ops[i].end_pos - ops[i].start_pos);
	            maxPos += delta;
	        }
	    }
	    maxPos += parser->maxLen;
	}
	
	if (maxPos > 0 && maxPos <= len) {//TODO: absolute offset
	    cout << "translate: len=" << len << " maxPos=" << maxPos << endl;
	    string oldcontent = string(d + maxPos, len - maxPos);
	    string newcontent = parser->getContentLengthExceed(oldcontent);
	    if (newcontent != oldcontent) {
	        memcpy(d + maxPos, newcontent.c_str(), newcontent.size());
	        len = len - oldcontent.size() + newcontent.size();
	    }
	}
	
	pkt->setTransportLen(len + hl);
	return len - len_old;
}

void NAT::doApp(PacketPtr pkt)
{
    DEST dest = pkt->getDEST();
    
	tcphdr* tcp = pkt->getTCPHeader();
	udphdr* udp = pkt->getUDPHeader();
	int len = pkt->getTransportLen();
	int hl = pkt->getTransportHeaderLen();
	FlowPtr flow = pkt->getFlow();
	if (!flow) return;
	if (tcp) {
	    int offsetc2s = flow->getOffset(SERVER);
	    int offsets2c = flow->getOffset(CLIENT);
        if (dest == SERVER) {
            tcp->th_seq = ntohl(ntohl(tcp->th_seq) + offsetc2s);
	        tcp->th_ack = ntohl(ntohl(tcp->th_ack) - offsets2c);
	    } else {
	        tcp->th_ack = ntohl(ntohl(tcp->th_ack) - offsetc2s);
            tcp->th_seq = ntohl(ntohl(tcp->th_seq) + offsets2c);
	    }
	}
	if (len <= hl || flow->ignored())
	    return;
	string content;
	if (tcp)
	    content = string((char*)tcp + hl, (char*)tcp + len);
	else if (udp)
	    content = string((char*)udp + hl, (char*)udp + len);
	if (content.size() <= 0)
	    return;
//	flow->save(content);
	string protocol = flow->getProtocol();
	vector<Operation> ops;
//	string chosenProtocol;
    ParserPtr parser;
	if (protocol.size() == 0) {
        std::map<std::string, StateManager::Protocol>::iterator it;
        for (it = sm.protocols.begin(); it != sm.protocols.end(); it++) {
            //check protocol 
            if (it->second.protocol == "tcp" && !pkt->isTCP())
                continue;
            if (it->second.protocol == "udp" && !pkt->isUDP())
                continue;printf("nat.cc #229: try to getParser:%s\n", it->first.c_str());
            parser = flow->getParser(it->first, dest);
            if (parser) {printf("found parser for %s\n", it->first.c_str());
                ops = parser->process(content);
                cout <<" process over: #"<< ops.size() << endl;
                if (ops.size() > 0) {
//                    chosenProtocol = it->first;
                    flow->setProtocol(it->first);
                    break;
                }
            } else {
                printf("not found parser for %s\n", it->first.c_str());
            }
        }
	} else {
	    std::cout << "flow " << flow << " 's protocol=" << protocol << std::endl;
	    parser = flow->getParser(protocol, dest);
        if (parser) {
            ops = parser->process(content);
        }
	}
	if (ops.size() > 0) {
        cout << "ops.size " << ops.size() << "operations!\n";
	    int delta = modify(pkt, ops, parser);
	    flow->addOffset(dest, delta);
	}
}

