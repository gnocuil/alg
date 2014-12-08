#include <cstdio>
#include <iostream>
#include <signal.h>
#include "state.h"
#include "socket.h"
#include "nfqueue.h"
#include "packet.h"
#include <net/ethernet.h>
#include "nat.h"

using namespace std;

static void sigHandler(int sig)
{
    exit(0);//TODO: fix the bug
    nfqueue_close();
    exit(0);
}

typedef struct pcap_hdr_s {
    uint32_t magic_number;   /* magic number */
    uint16_t version_major;  /* major version number */
    uint16_t version_minor;  /* minor version number */
    int32_t  thiszone;       /* GMT to local correction */
    uint32_t sigfigs;        /* accuracy of timestamps */
    uint32_t snaplen;        /* max length of captured packets, in octets */
    uint32_t network;        /* data link type */
} pcap_hdr_t;

typedef struct pcaprec_hdr_s {
    uint32_t ts_sec;         /* timestamp seconds */
    uint32_t ts_usec;        /* timestamp microseconds */
    uint32_t incl_len;       /* number of octets of packet saved in file */
    uint32_t orig_len;       /* actual length of packet */
} pcaprec_hdr_t;

void analyze(string filename)
{
    sm.init("example.conf");
    sm.analysisMode = true;
    cout << "analyze " << filename << endl;
    pcap_hdr_t header;
    FILE *file = fopen(filename.c_str(), "rb");
    int count = fread(&header, 1, sizeof(header), file);
    //cout << "count=" << count << endl;
    if (count != sizeof(header)) {
        cerr << "Error reading header in " << filename << endl;
        return;
    }
    printf("magic=%x\n", header.magic_number);
    printf("snaplen=%d\n", header.snaplen);
    printf("network=%d\n", header.network);
    pcaprec_hdr_t pkthdr;
    uint8_t buf[65536];
    int cnt = 0;
    long long total = count;
    do {
        count = fread(&pkthdr, 1, sizeof(pkthdr), file);
        if (count == 0) break;//EOF
        if (count != sizeof(pkthdr)) {
            cerr << "Error reading packet header in " << filename << endl;
            break;
        }
        total += count;
        //printf("time=[%d,%d]   incl_len=%d    orig_len=%d\n", pkthdr.ts_sec, pkthdr.ts_usec, pkthdr.incl_len, pkthdr.orig_len);
        count = fread(buf, 1, pkthdr.incl_len, file);
        if (count != pkthdr.incl_len) {
            cerr << "Error reading packet data in " << filename << endl;
            break;
        }
        total += count;
        //for (int i = 0; i < 40; ++i) printf("%02x ", buf[i]);printf("\n");
        ++cnt;

        PacketPtr pkt = PacketPtr(new Packet);
        pkt->ibuf_ = (buf + 14);
        pkt->setIbufLen(pkthdr.incl_len - 14);
        pkt->hw_protocol = ETHERTYPE_IP;
        pkt->unpack();        if (!pkt->isTCP()) continue; if (ntohs(pkt->getTCPHeader()->source) != 80 && ntohs(pkt->getTCPHeader()->dest) != 80) continue;
        cout << pkt->getDest4() << "   ";
        cout << "protocol=" << (int)(pkt->getIPHeader()->protocol) <<"   iplen=" << pkthdr.incl_len - 14 << "    ";
        if (pkt->isTCP()) {
            cout << "isTCP!  ";
            tcphdr* tcp = pkt->getTCPHeader();
            cout << (int)ntohs(tcp->source) << "  " << ntohs((int)tcp->dest) << " ";
        }
        cout<<endl;

        nat.translate(pkt);
                                                                                                          
        if (cnt > 10) break;
        
    } while (true);
    cout << "total packets=" << cnt << "    total Kbytes=" << total << endl;
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            //puts(argv[i]);
            analyze(argv[i]);
        }
        return 0;
    }
    struct sigaction s;
    struct sigaction t;
    s.sa_handler = sigHandler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    sigaction(SIGINT, &s, &t);

    sm.init("example.conf");
	init_socket();
	int rv;
	char buf[4096] __attribute__ ((aligned));
	int fd = nfqueue_init();

	for (;;) {
		if ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) {
			nfqueue_handle(buf, rv);
			continue;
		}
		if (rv < 0 && errno == ENOBUFS) {
			printf("losing packets!\n");
			continue;
		}
		perror("recv failed");
		break;
	}
	nfqueue_close();
	return 0;
}
