#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>		/* for NF_ACCEPT */
#include <cerrno>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <map>
#include "state.h"
#include "packet.h"
#include "socket.h"
#include "nat.h"

using namespace std;

SocketRaw4 socket4;
SocketRaw6 socket6;

StateManager sm;
NAT nat(sm);

/* returns packet id */
static PacketPtr print_pkt (struct nfq_data *tb)
{
    PacketPtr pkt = PacketPtr(new Packet);
	struct nfqnl_msg_packet_hdr *ph;

	ph = nfq_get_msg_packet_hdr(tb);
	if (ph) {
		pkt->id = ntohl(ph->packet_id);
		pkt->hw_protocol = ntohs(ph->hw_protocol);
	}

	pkt->setIbufLen(nfq_get_payload(tb, pkt->getIbuf()));
    return pkt;
}

void init_socket()
{
    socket4.init();
    socket6.init();
}

static map<int, int> offset;

static int process_app(PacketPtr pkt, int c2s)
{
	tcphdr* tcp = pkt->getTCPHeader();
	int len = pkt->getTransportLen();
	int hl = tcp->th_off * 4;
	
	const int key = int(tcp->th_sport) * tcp->th_dport;
	
	if (offset[key] != 0) {
	    if (c2s) {
	        tcp->th_seq = ntohl(ntohl(tcp->th_seq) + offset[key]);
	    } else {
	        tcp->th_ack = ntohl(ntohl(tcp->th_ack) - offset[key]);
	    }
	}
	
	//--printf("p_app: len=%d hl=%d th_off=%d sport=%d dport=%d c2s=%d key=%d offset=%d\n", len, hl, tcp->th_off, ntohs(tcp->th_sport), ntohs(tcp->th_dport), c2s, key, offset[key]);

	if (len > hl) {
	    string str((char*)tcp + hl, (char*)tcp + len);
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
	        
	        IP4Port ip4p = sm.doMapping(IP6Port(IPv6Addr(buf1), ntohs(port)));
	        
	        static char buf[2000] = {0};
	        sprintf(buf, "EPRT |1|%s|%d|\r\n", ip4p.getIP().getString().c_str(), ntohs(ip4p.getPort()));
	        int newlen = strlen(buf);
	        memcpy((char*)tcp + hl, buf, newlen);
	        newlen += hl;
	        offset[key] += newlen - len;
	        printf("newlen=%d buf=%s key=%d offset=%d\n", newlen, buf, key, offset[key]);
	        pkt->setTransportLen(newlen);
	        return newlen;
	    }
	}
	return len;
}

static int translate6to4(PacketPtr pkt)
{
    printf("translate6->4! len=%d\n", pkt->ibuf_len_);
    if (!sm.isAddrInRange(pkt->getDest6())) {
        //cout << "IPv6 address " << IPv6Addr(ip6->ip6_dst) << " not in range! return" << endl;
        return 0;
    }
    if (!pkt->isTCP()) {
        return 0;
    }
    
    nat.begin(pkt);
    nat.doSPort(pkt);
    process_app(pkt, 1);
    nat.doIP(pkt);
    nat.finish(pkt);
    socket4.send(pkt->obuf_, pkt->getObufLen(), pkt->getDest4());
    return 1;
}

static int translate4to6(PacketPtr pkt)
{
    printf("translate4->6! len=%d\n", pkt->ibuf_len_);
    iphdr* ip = pkt->getIPHeader();
    if (!sm.isAddrInRange(pkt->getDest4())) {
        //cout << "IPv4 address " << IPv4Addr(ip->daddr) << " not in range! return" << endl;
        return 0;
    }
    if (!pkt->isTCP()) {
        return 0;
    }
    nat.begin(pkt);
    process_app(pkt, 0);
    nat.doDPort(pkt);
    nat.doIP(pkt);
    nat.finish(pkt);
    socket6.send(pkt->obuf_, pkt->getObufLen(), pkt->getDest6());
    return 1;
}

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
	      struct nfq_data *nfa, void *data)
{
	//u_int32_t id = print_pkt(nfa);
	PacketPtr pkt = print_pkt(nfa);
	pkt->unpack();

    if (pkt->hw_protocol == ETHERTYPE_IPV6) {
        if (translate6to4(pkt))
            return nfq_set_verdict(qh, pkt->id, NF_DROP, 0, NULL);
    } else if (pkt->hw_protocol == ETHERTYPE_IP) {
        if (translate4to6(pkt))
            return nfq_set_verdict(qh, pkt->id, NF_DROP, 0, NULL);
    }
	return nfq_set_verdict(qh, pkt->id, NF_ACCEPT, 0, NULL);
}

int main(int argc, char **argv)
{
    sm.addIPv4Pool(IPv4Addr("10.20.30.40"));
    sm.setIPv6Prefix(IPv6Addr("2002::0"));

	struct nfq_handle *h;
	struct nfq_q_handle *qh;
	struct nfnl_handle *nh;
	int fd;
	int rv;
	char buf[4096] __attribute__ ((aligned));

	printf("opening library handle\n");
	h = nfq_open();
	if (!h) {
		fprintf(stderr, "error during nfq_open()\n");
		exit(1);
	}

	printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
	if (nfq_unbind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_unbind_pf()\n");
		exit(1);
	}

	printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
	if (nfq_bind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_bind_pf()\n");
		exit(1);
	}

	printf("binding this socket to queue '0'\n");
	qh = nfq_create_queue(h,  0, &cb, NULL);
	if (!qh) {
		fprintf(stderr, "error during nfq_create_queue()\n");
		exit(1);
	}

	printf("setting copy_packet mode\n");
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
		fprintf(stderr, "can't set packet_copy mode\n");
		exit(1);
	}

	fd = nfq_fd(h);
	
	init_socket();

	for (;;) {
		if ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) {
			nfq_handle_packet(h, buf, rv);
			continue;
		}
		/* if your application is too slow to digest the packets that
		 * are sent from kernel-space, the socket buffer that we use
		 * to enqueue packets may fill up returning ENOBUFS. Depending
		 * on your application, this error may be ignored. Please, see
		 * the doxygen documentation of this library on how to improve
		 * this situation.
		 */
		if (rv < 0 && errno == ENOBUFS) {
			printf("losing packets!\n");
			continue;
		}
		perror("recv failed");
		break;
	}

	printf("unbinding from queue 0\n");
	nfq_destroy_queue(qh);

#ifdef INSANE
	/* normally, applications SHOULD NOT issue this command, since
	 * it detaches other programs/sockets from AF_INET, too ! */
	printf("unbinding from AF_INET\n");
	nfq_unbind_pf(h, AF_INET);
#endif

	printf("closing library handle\n");
	nfq_close(h);

	exit(0);
}
