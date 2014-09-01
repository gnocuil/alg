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

using namespace std;

State s;

struct PktAttri {
    u_int32_t id;
    u_int32_t indev;
    u_int32_t outdev;
    u_int16_t hw_protocol;
    int len;
    unsigned char *data;
};

int send4_fd;
int send6_fd;

/* returns packet id */
static PktAttri print_pkt (struct nfq_data *tb)
{
    PktAttri att;
	struct nfqnl_msg_packet_hdr *ph;
	struct nfqnl_msg_packet_hw *hwph;
	u_int32_t mark,ifi; 

	ph = nfq_get_msg_packet_hdr(tb);
	if (ph) {
		att.id = ntohl(ph->packet_id);
		att.hw_protocol = ntohs(ph->hw_protocol);
		printf("hw_protocol=0x%04x hook=%u id=%u ",
			ntohs(ph->hw_protocol), ph->hook, att.id);
	}

	hwph = nfq_get_packet_hw(tb);
	if (hwph) {
		int i, hlen = ntohs(hwph->hw_addrlen);

		printf("hw_src_addr=");
		for (i = 0; i < hlen-1; i++)
			printf("%02x:", hwph->hw_addr[i]);
		printf("%02x ", hwph->hw_addr[hlen-1]);
	}

	mark = nfq_get_nfmark(tb);
	if (mark)
		printf("mark=%u ", mark);

	att.indev = nfq_get_indev(tb);
	if (ifi)
		printf("indev=%u ", att.indev);

	att.outdev = nfq_get_outdev(tb);
	if (ifi)
		printf("outdev=%u ", att.outdev);
	ifi = nfq_get_physindev(tb);
	if (ifi)
		printf("physindev=%u ", ifi);

	ifi = nfq_get_physoutdev(tb);
	if (ifi)
		printf("physoutdev=%u ", ifi);

	att.len = nfq_get_payload(tb, &att.data);
	if (att.len >= 0) {
		printf("payload_len=%d ", att.len);
    }

	fputc('\n', stdout);

	return att;
}

#define IP6_PREFIX "2002::192.168.2.4"

#define IP6_CLIENT "2001::2"

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

static void make_ip4pkt(u_int8_t* header, u_int16_t payload_len)
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
	u_int8_t addrbuf[16];
	inet_pton(AF_INET6, IP6_PREFIX, addrbuf);
	memcpy(&(ip->daddr), addrbuf + 12, 4);
	inet_pton(AF_INET, "192.168.2.1", &(ip->saddr));
	tcphdr* tcp = (tcphdr*)(header + 20);
	tcp->check = 0;
	tcp->check = tcp_checksum(tcp, payload_len, &(ip->saddr), &(ip->daddr), 4);
	ip->check = ip_checksum(ip, payload_len + 20);
}

static void make_ip6pkt(u_int8_t* header, u_int16_t payload_len)
{
    memset(header, 0, 20);
    
    ip6_hdr* ip6 = (ip6_hdr*)header;
    ip6->ip6_flow = 0x60;
    ip6->ip6_plen = ntohs(payload_len);
    ip6->ip6_hops = 64;
    ip6->ip6_nxt = 0x06;
    //ip->saddr = 
    //ip->daddr = 
	inet_pton(AF_INET6, IP6_PREFIX, &(ip6->ip6_src));
	inet_pton(AF_INET6, IP6_CLIENT, &(ip6->ip6_dst));
	tcphdr* tcp = (tcphdr*)(header + 40);
	tcp->check = 0;
	tcp->check = tcp_checksum(tcp, payload_len, &(ip6->ip6_src), &(ip6->ip6_dst), 16);
}

int send4(u_int8_t* buf, int len) {
	struct sockaddr_in dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	//memcpy(&dest.sin6_addr, buf + 24, 16);
	u_int8_t addrbuf[16];
	inet_pton(AF_INET6, IP6_PREFIX, addrbuf);
	memcpy(&dest.sin_addr, addrbuf + 12, 4);
	
	//if (sendto(send4_fd, buf, len, 0, (struct sockaddr *)&device, sizeof(device)) != len) {
	if (sendto(send4_fd, buf, len, 0, (struct sockaddr *)&dest, sizeof(dest)) != len) {
		fprintf(stderr, "socket_send: Failed to send ipv4 packet len=%d %s\n", len, strerror(errno));
		//for (int i = 0; i < len; ++i) printf("%d:%x ", i + 1, buf[i] & 0xFF);printf("\n");
		return -1;
	}
	return 0;
}

int send6(u_int8_t* buf, int len) {
	struct sockaddr_in6 dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin6_family = AF_INET6;
	inet_pton(AF_INET6, IP6_CLIENT, &dest.sin6_addr);

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

static int process_app(uint8_t *tcphead, int len, int c2s)
{
    
    
	tcphdr* tcp = (tcphdr*)(tcphead);
	int hl = tcp->th_off * 4;
	const int key = int(tcp->th_sport) * tcp->th_dport;
	
	if (offset[key] != 0) {
	    if (c2s) {
	        tcp->th_seq = ntohl(ntohl(tcp->th_seq) + offset[key]);
	    } else {
	        tcp->th_ack = ntohl(ntohl(tcp->th_ack) - offset[key]);
	    }
	}
	printf("p_app: len=%d hl=%d th_off=%d sport=%d dport=%d c2s=%d key=%d offset=%d\n", len, hl, tcp->th_off, ntohs(tcp->th_sport), ntohs(tcp->th_dport), c2s, key, offset[key]);
	printf("p_app_key1=%d\n", key);

	if (len > hl) {
	    string str(tcphead + hl, tcphead + len);
	    	printf("p_app_key2=%d\n", key);
	    if (str.size() > 5 && str.substr(0, 4) == "EPRT") {
	    	printf("p_app_key3=%d\n", key);
    	    cout << "str:[" << str << "]" << endl;
	        int p;
	        int port;
	        int cnt = 0;
	        for (int i = 0; i < str.size(); ++i) {
	            if (str[i] == '|') {
	                str[i] = ' ';
	                ++cnt;
	            }
	            if (cnt == 2) str[i] = ' ';
	        }
	        printf("p_app_key5=%d\n", key);
	        sscanf(str.c_str(), "EPRT %d %d", &p, &port);
	        printf("p=%d port=%d\n", p, port);
	        printf("p_app_key6=%d\n", key);
	        static char buf[2000] = {0};
	        sprintf(buf, "EPRT |1|192.168.2.1|%d|\r\n", port);
	        printf("p_app_key7=%d\n", key);
	        int newlen = strlen(buf);
	        memcpy(tcphead + hl, buf, newlen);
	        printf("p_app_key8=%d\n", key);
	        newlen += hl;
	        offset[key] += newlen - len;
	        	printf("p_app_key4=%d\n", key);
	        printf("newlen=%d buf=%s key=%d offset=%d\n", newlen, buf, key, offset[key]);
	        return newlen;
	    }
	}
	return len;
}

static int translate6to4(PktAttri att)
{
    int ip4_len = 20;
    int ip6_len = 40;
    static u_int8_t buf[2000];
    
    
    printf("translate6->4! len=%d\n", att.len);
    ip6_hdr* ip6 = (ip6_hdr*)att.data;
    if (ip6->ip6_nxt != 0x06) {
        return 0;
    }
    memcpy(buf + ip4_len, att.data + ip6_len, att.len - ip6_len);
    int tcplen = process_app(buf + ip4_len, att.len - ip6_len, 1);
    
    make_ip4pkt(buf, tcplen);

    send4(buf, tcplen + ip4_len);
    
    return 1;
}

static int translate4to6(PktAttri att)
{
    int ip4_len = 20;
    int ip6_len = 40;
    static u_int8_t buf[2000];
    
    
    printf("translate4->6! len=%d\n", att.len);
    iphdr* ip = (iphdr*)att.data;
    if (ip->protocol != 0x06) {
        return 0;
    }
    memcpy(buf + ip6_len, att.data + ip4_len, att.len - ip4_len);
    process_app(buf + ip6_len, att.len - ip4_len, 0);
    make_ip6pkt(buf, att.len - ip4_len);

    send6(buf, att.len - ip4_len + ip6_len);
    
    return 1;
}

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
	      struct nfq_data *nfa, void *data)
{
	//u_int32_t id = print_pkt(nfa);
	PktAttri att = print_pkt(nfa);
	//printf("entering callback\n");

    if (att.hw_protocol == ETHERTYPE_IPV6) {
        if (translate6to4(att))
            return nfq_set_verdict(qh, att.id, NF_DROP, 0, NULL);
    } else if (att.hw_protocol == ETHERTYPE_IP) {
        if (translate4to6(att))
            return nfq_set_verdict(qh, att.id, NF_DROP, 0, NULL);
    }
	return nfq_set_verdict(qh, att.id, NF_ACCEPT, 0, NULL);
}

int main(int argc, char **argv)
{
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
			printf("pkt received\n");
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
