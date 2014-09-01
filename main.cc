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

using namespace std;

int send4_fd;
int send6_fd;

StateManager sm;

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

uint16_t tcp_checksum(const void *tcphead, size_t tcplen, const void* saddr, const void*  daddr, int addrlen)
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

uint16_t ip_checksum(const void *iphead, size_t iplen)
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

static void make_ip4pkt(u_int8_t* header, u_int16_t payload_len, const IPv4Addr& ip4saddr, const IPv4Addr& ip4daddr)
{
    memset(header, 0, 20);
    iphdr* ip = (iphdr*)header;
//    header[0] = 0x45;
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

	tcphdr* tcp = (tcphdr*)(header + 20);
	tcp->check = 0;
	tcp->check = tcp_checksum(tcp, payload_len, &(ip->saddr), &(ip->daddr), 4);
	ip->check = ip_checksum(ip, payload_len + 20);
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

	tcphdr* tcp = (tcphdr*)(header + 40);
	tcp->check = 0;
	tcp->check = tcp_checksum(tcp, payload_len, &(ip6->ip6_src), &(ip6->ip6_dst), 16);
}

int send4(u_int8_t* buf, int len, const IPv4Addr& daddr) {
	struct sockaddr_in dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	uint32_t daddrbuf = daddr.getInt();
	memcpy(&dest.sin_addr, &daddrbuf, 4);
	
	//if (sendto(send4_fd, buf, len, 0, (struct sockaddr *)&device, sizeof(device)) != len) {
	if (sendto(send4_fd, buf, len, 0, (struct sockaddr *)&dest, sizeof(dest)) != len) {
		fprintf(stderr, "socket_send: Failed to send ipv4 packet len=%d %s\n", len, strerror(errno));
		//for (int i = 0; i < len; ++i) printf("%d:%x ", i + 1, buf[i] & 0xFF);printf("\n");
		return -1;
	}
	return 0;
}

int send6(u_int8_t* buf, int len, const IPv6Addr& daddr) {
	struct sockaddr_in6 dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin6_family = AF_INET6;
	dest.sin6_addr = daddr.getIn6Addr();
	//inet_pton(AF_INET6, IP6_CLIENT, &dest.sin6_addr);

	if (sendto(send6_fd, buf, len, 0, (struct sockaddr *)&dest, sizeof(dest)) != len) {
		fprintf(stderr, "socket_send: Failed to send ipv6 packet len=%d %s\n", len, strerror(errno));
		//for (int i = 0; i < len; ++i) printf("%d:%x ", i + 1, buf[i] & 0xFF);printf("\n");
		return -1;
	}
	return 0;
}

void init_socket()
{
    send4_fd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
    if (send4_fd < 0) {
        perror("Error in socket() for send4_fd");
        exit(1);
    }
    send6_fd = socket(PF_INET6, SOCK_RAW, IPPROTO_RAW);
    if (send6_fd < 0) {
        perror("Error in socket() for send4_fd");
        exit(1);
    }
}

static map<int, int> offset;

static int process_app(uint8_t *tcphead, int len, int c2s, uint16_t sport, uint16_t dport)
{
	tcphdr* tcp = (tcphdr*)(tcphead);
	int hl = tcp->th_off * 4;
	
	if (sport > 0) {
	    tcp->source = sport;
	}
	
	const int key = int(tcp->th_sport) * tcp->th_dport;
	
	if (offset[key] != 0) {
	    if (c2s) {
	        tcp->th_seq = ntohl(ntohl(tcp->th_seq) + offset[key]);
	    } else {
	        tcp->th_ack = ntohl(ntohl(tcp->th_ack) - offset[key]);
	    }
	}
	
	if (dport > 0) {
	    tcp->dest = dport;
	}
	
	//--printf("p_app: len=%d hl=%d th_off=%d sport=%d dport=%d c2s=%d key=%d offset=%d\n", len, hl, tcp->th_off, ntohs(tcp->th_sport), ntohs(tcp->th_dport), c2s, key, offset[key]);

	if (len > hl) {
	    string str(tcphead + hl, tcphead + len);
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
	        memcpy(tcphead + hl, buf, newlen);
	        newlen += hl;
	        offset[key] += newlen - len;
	        printf("newlen=%d buf=%s key=%d offset=%d\n", newlen, buf, key, offset[key]);
	        return newlen;
	    }
	}
	return len;
}

static int translate6to4(PacketPtr pkt)
{
    int ip4_len = 20;
    int ip6_len = 40;
    static u_int8_t buf[2000];
    
    
    printf("translate6->4! len=%d\n", pkt->ibuf_len_);
    ip6_hdr* ip6 = (ip6_hdr*)pkt->ibuf_;
    
    if (!sm.isAddrInRange(IPv6Addr(ip6->ip6_dst))) {
        cout << "IPv6 address " << IPv6Addr(ip6->ip6_dst) << " not in range! return" << endl;
        return 0;
    }
    
    if (ip6->ip6_nxt != 0x06) {
        return 0;
    }
    
    tcphdr* tcp = (tcphdr*)(pkt->ibuf_ + ip6_len);
    IP4Port ip4p = sm.doMapping(IP6Port(IPv6Addr(ip6->ip6_src), tcp->source));
    
    IPv4Addr daddr = sm.getServerAddr(IPv6Addr(ip6->ip6_dst));
    
    memcpy(buf + ip4_len, pkt->ibuf_ + ip6_len, pkt->ibuf_len_ - ip6_len);
    int tcplen = process_app(buf + ip4_len, pkt->ibuf_len_ - ip6_len, 1, ip4p.getPort(), 0);
    
    make_ip4pkt(buf, tcplen, ip4p.getIP(), daddr);

    send4(buf, tcplen + ip4_len, daddr);
    
    return 1;
}

static int translate4to6(PacketPtr pkt)
{
    int ip4_len = 20;
    int ip6_len = 40;
    static u_int8_t buf[2000];
    
    //printf("translate4->6! len=%d\n", att.len);
    iphdr* ip = (iphdr*)pkt->ibuf_;
    
    if (!sm.isAddrInRange(IPv4Addr(ip->daddr))) {
        //puts("IPv4 address not in range! return");
        return 0;
    }
    
    if (ip->protocol != 0x06) {
        return 0;
    }
    
    tcphdr* tcp = (tcphdr*)(pkt->ibuf_ + ip4_len);
    IP6Port ip6p = sm.getMapping(IP4Port(IPv4Addr(ip->daddr), tcp->dest));
    
    IPv6Addr saddr = sm.getServerAddr(IPv4Addr(ip->saddr));
    
    memcpy(buf + ip6_len, pkt->ibuf_ + ip4_len, pkt->ibuf_len_ - ip4_len);
    process_app(buf + ip6_len, pkt->ibuf_len_ - ip4_len, 0, 0, ip6p.getPort());
    make_ip6pkt(buf, pkt->ibuf_len_ - ip4_len, saddr, ip6p.getIP());

    send6(buf, pkt->ibuf_len_ - ip4_len + ip6_len, ip6p.getIP());
    
    return 1;
}

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
	      struct nfq_data *nfa, void *data)
{
	//u_int32_t id = print_pkt(nfa);
	PacketPtr pkt = print_pkt(nfa);

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
/*
    cout << IPv6Addr("2001::3") << endl;

    cout << sm.doMapping(IP6Port(IPv6Addr("2001::3"), 12345)) << endl;
    
    cout << sm.doMapping(IP6Port(IPv6Addr("2001::3"), 12346)) << endl;
    
    cout << sm.doMapping(IP6Port(IPv6Addr("2001::3"), 12345)) << endl;
    
    cout << sm.getServerAddr(IPv4Addr("3.4.5.6")) << endl;
    cout << sm.getServerAddr(IPv6Addr("2002::8.7.6.5")) << endl;
    
    cout << "isInRange:2002::88.77.66.55 " << sm.isAddrInRange(IPv6Addr("2002::88.77.66.55")) << endl;
    cout << "isInRange:2003::88.77.66.55 " << sm.isAddrInRange(IPv6Addr("2003::88.77.66.55")) << endl;
    
    return 0;
*/
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
