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

#include "communicator.h" //TODO: remove

using namespace std;

SocketRaw4 socket4;
SocketRaw6 socket6;

StateManager sm;
//NAT nat(sm);
NAT nat;

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
    //pkt->print();
    
    nat.begin(pkt);
    nat.doApp(pkt);
    nat.doSPort(pkt);
    nat.doIP(pkt);
    nat.finish(pkt);
    socket4.send(pkt->obuf_, pkt->getObufLen(), pkt->getDest4());
    return 1;
}

static int translate4to6(PacketPtr pkt)
{
    printf("translate4->6! len=%d\n", pkt->ibuf_len_);
//    iphdr* ip = pkt->getIPHeader();
    if (!sm.isAddrInRange(pkt->getDest4())) {
        //cout << "IPv4 address " << IPv4Addr(ip->daddr) << " not in range! return" << endl;
        return 0;
    }
    if (!pkt->isTCP()) {
        return 0;
    }
    //pkt->print();
    
    nat.begin(pkt);
    nat.doApp(pkt);
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
/*    
    StatefulCommunicator sc;
    sc.addData("Hello Comm!!");
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
