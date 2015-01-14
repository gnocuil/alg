#include "packet.h"
#include <linux/if_tun.h>
#include <net/if.h>
#include <netinet/in.h>
#include <cstdio>
#include "nat.h"
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <queue>


static int tun_fd;

const int T = 4;//threads
const int QMAX = 10000;//max packets in a queue
std::queue<PacketPtr> q[T];

pthread_mutex_t mutex[T];
int ii[T];

void* thread_t(void* arg)
{
    int tid = *(int*)arg;
    printf("run tid %d\n", tid);
    
    NAT nat;
    nat.tid = tid;

    do {
        PacketPtr pkt = PacketPtr();
        pthread_mutex_lock(&mutex[tid]);
        if (!q[tid].empty()) {
            pkt = q[tid].front();
            q[tid].pop();
        }
        pthread_mutex_unlock(&mutex[tid]);
        if (pkt) {
//printf("\tPop packet: %d %d len=%d tid=%d\n", ntohs(pkt->getSourcePort()), ntohs(pkt->getDestPort()), pkt->getTransportLen(), tid);

            nat.translate(pkt);
            
        } else {
            usleep(1);
        }
    } while (true);

    return NULL;
}

int tun_create(char *dev)
{
	struct ifreq ifr;
	int err;

	if ((tun_fd = open("/dev/net/tun", O_RDWR)) < 0) {
		fprintf(stderr, "tun_create: Error Creating TUN/TAP: %m\n", errno);
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags |= IFF_TUN | IFF_NO_PI;

	if (*dev != '\0') {
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	}

	if ((err = ioctl(tun_fd, TUNSETIFF, (void *)&ifr)) < 0) {
		fprintf(stderr, "tun_create: Error Setting tunnel name %s: %m\n", dev, errno);
		close(tun_fd);
		return -1;
	}
/*
	if (fcntl(tun_fd, F_SETFL, O_NONBLOCK) < 0) {
		fprintf(stderr, "tun_create: Error Setting nonblock: %m\n", dev, errno);
		return -1;
	}
*/
	
	strcpy(dev, ifr.ifr_name);

    for (int i = 0; i < T; ++i) {
        ii[i] = i;
        mutex[i] = PTHREAD_MUTEX_INITIALIZER;
        pthread_t tid;
        pthread_create(&tid, NULL, thread_t, (void*) (&ii[i])   );
    }

	
	return tun_fd;
}

void tun_handle(char *buf, int len)
{
    PacketPtr pkt = PacketPtr(new Packet);
    if (buf[0] == 0x60)
        pkt->hw_protocol = ETHERTYPE_IPV6;
    else
        pkt->hw_protocol = ETHERTYPE_IP;
    pkt->setIbuf((uint8_t*)buf, len);
//    pkt->setIbufLen(len);
    pkt->unpack();
    
    int tid;
    FlowPtr f = pkt->getFlow();
    if (f && f->tid >= 0) {
        tid = f->tid;
        //std::cout << "\tget " << f << "  tid " << tid << std::endl;
    } else tid = pkt->port_t % T;
    //printf("imcoming packet: %d %d %d len=%d tid=%d\n", ntohs(pkt->getSourcePort()), ntohs(pkt->getDestPort()), pkt->port_t, pkt->getTransportLen(), tid);
    
    while (q[tid].size() > QMAX) {
        //printf("too much in queue %d\n", tid);
        usleep(1);
    }
    pthread_mutex_lock(&mutex[tid]);
    q[tid].push(pkt);
    pthread_mutex_unlock(&mutex[tid]);
	//nat.translate(pkt);
}


