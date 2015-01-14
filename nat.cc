#include <cstring>
#include <iostream>
#include <net/ethernet.h>
#include "socket.h"
#include "nat.h"
#include "ip.h"
#include "state.h"

using namespace std;

//NAT nat;

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

bool NAT::doExtraTCP(PacketPtr pkt)
{
    DEST dest = pkt->getDEST();
    
    tcphdr* tcp = pkt->getTCPHeader();
    int len = pkt->getTransportLen();
    int hl = pkt->getTransportHeaderLen();

    if (!exttcpflow_) return false;
    if (!tcp) return false;
    if (len <= hl || exttcpflow_->ignored()) {
        return false;
    }
    int newlen = exttcpflow_->trail[dest].size();
    if (newlen > 0) {
        memcpy((char*)tcp + hl, exttcpflow_->trail[dest].c_str(), newlen);

        pkt->setTransportLen(newlen + hl);
        tcp->th_seq = ntohl(ntohl(tcp->th_seq) + len - hl);
        exttcpflow_->addOffset(dest, newlen);

        exttcpflow_->trail[dest] = "";
        return true;
    }
    return false;
}

int NAT::translate(PacketPtr pkt)
{
    sm.count1("total");
    sm.count1("begin");
    if (!sm.analysisMode) {
        if (pkt->hw_protocol == ETHERTYPE_IPV6) {
            if (!sm.isAddrInRange(pkt->getDest6()))
                return 0;
        } else {
            if (!sm.isAddrInRange(pkt->getDest4()))
                return 0;
        }
    }
    if (!pkt->isTCP() && !pkt->isUDP()) {
        return 0;
    }//printf("  translate: %d %d  len=%d tid=%d\n", ntohs(pkt->getSourcePort()), ntohs(pkt->getDestPort()), pkt->getTransportLen(), tid);
    if (!sm.analysisMode) {
        if (begin(pkt))
            return 0;
    }

    modified_ = false;
    sm.count2("begin");
    sm.count1("app");
    try {
        //doApp(pkt);
    } catch (const std::exception& ex) {
        cerr << "Exception in doApp: " << ex.what() << endl;
    } catch (...) {
        cerr << "Exception in doApp: ..." << endl;
    }
    sm.count2("app");
    if (sm.analysisMode)
        return 1;
    sm.count1("nat64");
    doSPort(pkt);
    doDPort(pkt);
    doIP(pkt);
    finish(pkt);
    sm.count2("nat64");
    sm.count2("total");
    if (!sm.analysisMode) {
        if (pkt->hw_protocol == ETHERTYPE_IPV6) {//printf("send4 tid=%d len=%d\n", tid, pkt->getObufLen());
            socket4.send(pkt->obuf_, pkt->getObufLen(), pkt->getDest4());
        } else {//printf("send6 tid=%d len=%d ", tid, pkt->getObufLen());cout << ip6p_ << endl;
            bool keep = modified_;
            if (pkt->getTransportLen() - pkt->getTransportHeaderLen() == 0) keep = true;
            //printf("flags=%x  transportlen=%d transporthlen=%d keep=%d\n", pkt->getTCPHeader()->th_flags, pkt->getTransportLen(), pkt->getTransportHeaderLen(), keep);
            
            if (sm.extra == TCP) {
                unsigned char buf[65535];
                int len = pkt->getObufLen();
                memcpy(buf, pkt->obuf_, len);
                if (doExtraTCP(pkt)) {
                    socket6.send(pkt->obuf_, pkt->getObufLen(), pkt->getDest6(), true);
                }
                socket6.send(buf, len, pkt->getDest6(), keep);
            } else {
                socket6.send(pkt->obuf_, pkt->getObufLen(), pkt->getDest6(), keep);
            }
        }
    }

    
//    cout << "Len=" << pkt->getObufLen() << "   total=" << sm.time["total"] << "    nat64=" << sm.time["nat64"] << "    app=" << sm.time["app"] << "    parse=" << sm.time["parse"] << "    mo=" << sm.time["mo"] << "    be=" << sm.time["begin"]     << endl;
    
    static long long total = 0;
    static long long app = 0;
    static long long n64 = 0;
    static long long parse = 0;
    static long long mo = 0;
    static long long be = 0;
    
    total += sm.time["total"];
    app += sm.time["app"];
    n64 += sm.time["nat64"];
    parse += sm.time["parse"]; sm.time["parse"] = 0;
    mo += sm.time["mo"];
    be += sm.time["begin"];
    
//    cout << "   total:" << "   total=" << total << "    nat64=" << n64 << "    app=" << app << "    parse=" << parse << "    mo=" << mo << "    be="<<be    << endl;
    
    return 1;
}

int NAT::begin(PacketPtr pkt)
{
    if (pkt->getIPVersion() == 4) {
        ip6_hdr* ip6 = pkt->getIP6Header();
        FlowPtr f = sm.doMapping(IP6Port(IPv6Addr(ip6->ip6_src), pkt->getSourcePort()), IPv6Addr(ip6->ip6_dst));
        if (f->tid < 0) {
            f->tid = tid;
            //cout << "set " <<f << "  tid=" << tid << endl;
        }
        ip4p_ = f->ip4p;
        sm.setCurIPv6SrvAddr(IPv6Addr(ip6->ip6_dst));
    } else {
        iphdr* ip = pkt->getIPHeader();
        FlowPtr f = sm.getMapping(IP4Port(IPv4Addr(ip->daddr), pkt->getDestPort()), IPv4Addr(ip->saddr));
        if (!f) return 1;
        //cout << "getflow: dport=" << ntohs(pkt->getDestPort()) << "   " << f << endl;
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

void NAT::doApp(PacketPtr pkt)
{
    DEST dest = pkt->getDEST();
    tcphdr* tcp = pkt->getTCPHeader();
    udphdr* udp = pkt->getUDPHeader();
    int len = pkt->getTransportLen();
    int hl = pkt->getTransportHeaderLen();
    FlowPtr flow = pkt->getFlow();
    exttcpflow_ = flow;
    if (!flow) return;
    if (sm.analysisMode) {
        iphdr* ip = pkt->getIPHeader();
        uint16_t dport = 0;
        if (tcp) dport = tcp->dest;
        else if (udp) dport = udp->dest;
        IP4Port ip4p = IP4Port(IPv4Addr(ip->daddr), dport);
        if (flow->ip4p == ip4p) {
            dest = SERVER;
        } else {
            dest = CLIENT;
        }
    }
    if (!sm.analysisMode) {
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
    }
    //shift
    if (sm.extra == SHIFT) {
        if (tcp)
            flow->shift(pkt, (char*)tcp + hl, len - hl);
        //else if (udp)
        //    flow->shift(pkt, (char*)udp + hl, len);
    }
    
    string content;
    if (tcp)
        content = string((char*)tcp + hl, (char*)tcp + len);
    else if (udp)
        content = string((char*)udp + hl, (char*)udp + len);
    if (content.size() <= 0)
        return;
//    printf("<CONTENT>");for (int i = 0; i < 50; ++i) putchar(content[i]);printf("</CONTENT>\n");
    if (pkt->isTCP()) {
        flow->count(pkt, dest);
    }
//    flow->save(content);
    string protocol = flow->getProtocol();
    vector<Operation> ops;
//    string chosenProtocol;
    ParserPtr parser;
    string protocol_guess;
    int protocol_guess_count = 0;sm.count1("parse");
    if (protocol.size() == 0) {
        std::map<std::string, StateManager::Protocol>::iterator it;
        for (it = sm.protocols.begin(); it != sm.protocols.end(); it++) {
            //check protocol 
            if (it->second.protocol == "tcp" && !pkt->isTCP())
                continue;
            if (it->second.protocol == "udp" && !pkt->isUDP())
                continue;
            //cout << it->first<<endl;
            parser = flow->getParser(it->first, dest);

            if (parser) {//printf("try process...\n");
                ops = parser->process(content);
//                if (ops.size > 0) cout <<" process over: #"<< ops.size() << endl << endl << endl;
                if (ops.size() > 0) {
//                    chosenProtocol = it->first;
                    flow->setProtocol(it->first);
                    protocol = it->first;
                    break;
                } else if (parser->isCorrectProtocol()) {
                    protocol_guess = it->first;
                    protocol_guess_count ++;
                }
            } else {
//                printf("not found parser for %s\n", it->first.c_str());
            }
        }
    } else {//puts("known protocol");
        parser = flow->getParser(protocol, dest);
        if (parser) {
            ops = parser->process(content);
        }
    }sm.count2("parse");
    if (protocol.size() == 0 && protocol_guess_count == 1) {
    //puts("set pro");
        flow->setProtocol(protocol_guess);
    }
    if (protocol.size() > 0) {
        //std::cout<<dest<<" "<<sm.protocols[protocol].ptype_s2c<<" " <<sm.protocols[protocol].ptype_c2s<<endl;
    }
sm.count1("mo");
    if (ops.size() > 0) {
        modified_ = true;
//        cout << "ops.size " << ops.size() << "operations!\n";
        int delta = flow->modify(pkt, ops, parser);
        flow->addOffset(dest, delta);
    }
    sm.count2("mo");
}

