#include <cstdio>
#include <iostream>
#include <signal.h>
#include "state.h"
#include "socket.h"
#include "nfqueue.h"
#include "packet.h"
#include <net/ethernet.h>
#include "nat.h"
#include "tun.h"

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

static long long gettime(struct timeval t1, struct timeval t2) {
    return (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec) ;
}

int parse(char *buf, int len) {
    for (int i = 0; i + 13 < len; ++i) {
        if (buf[i] == 'h' && buf[i+1] == 't' && buf[i+2] == 't' && buf[i+3] == 'p' && buf[i+4] == ':' && buf[i+5] == '/' && buf[i+6] == '/') {
          try{
            int j = i + 7, t;
            //#1
            for (t = 0; t < 3; ++t)
                if (j + t >= len || (buf[j+t]<'0' || buf[j+t]>'9'))
                    break;
            if (t <= 0) continue;
            j += t;
            if (j >= len || buf[j] != '.') 
                continue;
            ++j;
            if (j >= len) 
                continue;
            //#2
            for (t = 0; t < 3; ++t)
                if (j + t >= len || (buf[j+t]<'0' || buf[j+t]>'9'))
                    break;
            if (t <= 0) continue;
            j += t;
            if (j >= len || buf[j] != '.') 
                continue;
            ++j;
            if (j >= len) 
                continue;
            
            //#3    
            for (t = 0; t < 3; ++t)
                if (j + t >= len || (buf[j+t]<'0' || buf[j+t]>'9'))
                    break;
            if (t <= 0) continue;
            j += t;
            if (j >= len || buf[j] != '.') 
                continue;
            ++j;
            if (j >= len) 
                continue;
                
            for (t = 0; t < 3; ++t)
                if (j + t >= len || (buf[j+t]<'0' || buf[j+t]>'9'))
                    break;
            if (t <= 0) continue;
            j += t;

            /*
                printf("found: ");
                for (int j = i; j < len && j < i + 50; ++j) putchar(buf[j]);
                printf("\n");
            */  
                return 1;
          } catch (...) {
                printf("Exception: ");
                for (int j = i; j < len && j < i + 50; ++j) putchar(buf[j]);
                printf("\n");            
          }

        }
    }
    return 0;
}

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
    int mxlen = 0;
    struct timeval t1;
    gettimeofday(&t1, NULL);
    struct timeval t3;
    gettimeofday(&t3, NULL);
    int total_flow = 0;
    int sip_flow = 0;
    int ftp_flow = 0;
    int http_flow = 0;
    int literal = 0;
    long long total1 = 0;
    do {
        count = fread(&pkthdr, 1, sizeof(pkthdr), file);
        if (count == 0) break;//EOF
        if (count != sizeof(pkthdr)) {
            cerr << "Error reading packet header in " << filename << endl;
            break;
        }
        //total += count;
        //printf("time=[%d,%d]   incl_len=%d    orig_len=%d\n", pkthdr.ts_sec, pkthdr.ts_usec, pkthdr.incl_len, pkthdr.orig_len);
        if (pkthdr.incl_len > mxlen) {
            mxlen = pkthdr.incl_len;
            printf("mxlen=%d\n", mxlen);
        }
        count = fread(buf, 1, pkthdr.incl_len, file);
        if (count != pkthdr.incl_len) {
            cerr << "Error reading packet data in " << filename << endl;
            break;
        }
        //printf("read count=%d\n", count);
        total += count;
        //for (int i = 14; i < 50; ++i) printf("%02x ", buf[i]);printf("\n");
        ++cnt;
        
        if (total1 == 0 && cnt > 2000000) {
            total1 = total;
            gettimeofday(&t1, NULL);
        }
        if (total1 > 0) 
            if (cnt % 100 == 0) usleep(5000);
        
        
        
        struct timeval t4;
        gettimeofday(&t4, NULL);
        if (gettime(t3, t4) > 1000000) {
            cout << "cnt="<<cnt<<endl;
            gettimeofday(&t3, NULL);
        }
        
        //if (cnt < 4488827) continue;
        
        if (buf[12] != 0x08 || buf[13] != 0x00) {//not ipv4
            //puts("not ipv4! ignore");
            continue;
        }
        PacketPtr pkt = PacketPtr(new Packet);
//        pkt->ibuf_ = (buf + 14);
        pkt->setIbuf(buf + 14, pkthdr.incl_len - 14);
        pkt->hw_protocol = ETHERTYPE_IP;
        pkt->unpack();        //if (!pkt->isTCP()) continue; if (ntohs(pkt->getTCPHeader()->source) != 80 && ntohs(pkt->getTCPHeader()->dest) != 80) continue;
//        cout << pkt->getDest4() << "   ";
//        cout << "protocol=" << (int)(pkt->getIPHeader()->protocol) <<"   iplen=" << pkthdr.incl_len - 14 << "    ";
        if (pkt->isTCP()) {
//            cout << "isTCP!  ";
            tcphdr* tcp = pkt->getTCPHeader();
//            cout << (int)ntohs(tcp->source) << "  " << ntohs((int)tcp->dest) << " ";
        }
//        cout<<endl;

        NAT nat(false);
        nat.translate(pkt);
        
        FlowPtr f = pkt->getFlow();
        if (f) {
            if (f->analyze == 0) {
                ++total_flow;
                f->analyze = 1;
            }
            if (f->analyze <= 1) {
                if (pkt->isTCP()) {
                    if (ntohs(pkt->getSourcePort()) == 80 || ntohs(pkt->getDestPort()) == 80) {//http
//                        printf("maybe http!\n");
                        f->analyze = 2;
                        ++http_flow;
                    }
                    if (ntohs(pkt->getSourcePort()) == 8080 || ntohs(pkt->getDestPort()) == 8080) {//http
//                        printf("maybe http!\n");
                        f->analyze = 2;
                        ++http_flow;
                    }
                    if (ntohs(pkt->getSourcePort()) == 21 || ntohs(pkt->getDestPort()) == 21) {//http
//                        printf("maybe ftp!\n");
                        f->analyze = 3;
                        ++ftp_flow;
                    }
                } else if (pkt->isUDP()) {
                    if (ntohs(pkt->getSourcePort()) == 5060 || ntohs(pkt->getDestPort()) == 5060) {//http
//                        printf("maybe sip!\n");
                        f->analyze = 4;
                        ++sip_flow;
                    }
                }
            }
            if (f->analyze == 2 || f->analyze == 100) {
                if (parse((char*)(buf + 14), pkthdr.incl_len - 14)) {
                    if (f->analyze == 2)
                        ++literal;
                    f->analyze = 100;
                    
                }
            }
        }
                                                                                                     
        if (cnt > 2200000) break;
        

        
    } while (true);
    struct timeval t2;
    gettimeofday(&t2, NULL);
    total -= total1;
    cout << "total packets=" << cnt << "    total Kbytes=" << total/1024 << "   #flows=" << sm.flow_cnt_ << endl;
    long long time = gettime(t1, t2);
    cout << "time=" << time/1000.0 << "ms  throughput(Mbps)=" << total * 8 * 1000000 / 1024 / 1024 / time  << endl;
    cout << "total_flow=" << total_flow << "  http=" << http_flow << "  ftp=" << ftp_flow << "  sip=" << sip_flow <<endl;
    cout << "literal_http_flow=" << literal << endl;
}

int main(int argc, char **argv)
{
    //sm.extra = TCP;
    //sm.extra = SHIFT;
    sm.extra = NONE;
    sm.tun = true;
    
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
	//init_socket();
	int rv;
	char buf[4096] __attribute__ ((aligned));
	int fd;
    char tun_name[100] = {0};
    if (!sm.tun)
        fd = nfqueue_init();
    else
        fd = tun_create(tun_name);
	
	for (;;) {
        if (!sm.tun) {
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
        } else {
        	rv = read(tun_fd, buf, sizeof(buf));
//printf("tun recv %d\n", rv);
        	if (rv < 0) {
                perror("recv_tun failed");
        		return -1;
            }
            tun_handle(buf, rv);
        }
	}
    if (!sm.tun) 
    	nfqueue_close();
	return 0;
}
