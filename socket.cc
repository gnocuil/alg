#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <cerrno>
#include <iostream>
#include <cstdlib>
#include "socket.h"

using namespace std;

//SocketRaw4 socket4;
//SocketRaw6 socket6;
/*
void init_socket()
{
    socket4.init();
    socket6.init();
}
*/

static pthread_mutex_t smutex = PTHREAD_MUTEX_INITIALIZER;
static void lock() {pthread_mutex_lock(&smutex);}
static void unlock() {pthread_mutex_unlock(&smutex);}

void Socket::init()
{    
    fd_ = initSocket();
    if (fd_ < 0) {
        perror("Socket Creation Error!");
        exit(1);
    }
}

int SocketRaw4::initSocket()
{
    return socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
}

int SocketRaw6::initSocket()
{
    len_ = 0;
    return socket(PF_INET6, SOCK_RAW, IPPROTO_RAW);
}

int SocketRaw4::send(u_int8_t* buf, int len, const IPv4Addr& daddr)
{ //cout << "send4 len=" << len << " dest=" << daddr << endl;
	struct sockaddr_in dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	uint32_t daddrbuf = daddr.getInt();
	memcpy(&dest.sin_addr, &daddrbuf, 4);
//    lock();	
	if (sendto(fd_, buf, len, 0, (struct sockaddr *)&dest, sizeof(dest)) != len) {
		fprintf(stderr, "socket4 send: Failed to send ipv4 packet len=%d %s\n", len, strerror(errno));
//		unlock();
		return -1;
	}
//	unlock();
	return 0;
}

int SocketRaw6::send(u_int8_t* buf, int len, const IPv6Addr& daddr, bool keep)
{ 
    if (len_ > 0) {
        //timeout();
    }
    if (0 && len > 100) {
        memcpy(buf_, buf, len);
        len_ = len;
        daddr_ = daddr;
        return 0;
    }

    /*
    int r = rand() % 100;
    if (!keep && r < 0) {
        //printf("drop:r=%d\n", r);
        return 0;
    }*/
	struct sockaddr_in6 dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin6_family = AF_INET6;
	dest.sin6_addr = daddr.getIn6Addr();
//cout << "send6 len=" << len << " dest=" << daddr << endl;
//    lock();
	if (sendto(fd_, buf, len, 0, (struct sockaddr *)&dest, sizeof(dest)) != len) {
		fprintf(stderr, "socket6 send: Failed to send ipv6 packet len=%d %s\n", len, strerror(errno));
//		unlock();
		return -1;
	}
//	unlock();
	return 0;
}

int SocketRaw6::timeout()
{if (len_<=0) return 0;
	struct sockaddr_in6 dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin6_family = AF_INET6;
	dest.sin6_addr = daddr_.getIn6Addr();

	if (sendto(fd_, buf_, len_, 0, (struct sockaddr *)&dest, sizeof(dest)) != len_) {
		fprintf(stderr, "socket6 send: Failed to send ipv6 packet len=%d %s\n", len_, strerror(errno));
		return -1;
	}
	return 0;
    len_ = 0;
}
