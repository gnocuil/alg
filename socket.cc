#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <cerrno>
#include <iostream>
#include "socket.h"

using namespace std;

SocketRaw4 socket4;
SocketRaw6 socket6;

void init_socket()
{
    socket4.init();
    socket6.init();
}

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
    return socket(PF_INET6, SOCK_RAW, IPPROTO_RAW);
}

int SocketRaw4::send(u_int8_t* buf, int len, const IPv4Addr& daddr)
{ //cout << "send4 len=" << len << " dest=" << daddr << endl;
	struct sockaddr_in dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	uint32_t daddrbuf = daddr.getInt();
	memcpy(&dest.sin_addr, &daddrbuf, 4);
	
	if (sendto(fd_, buf, len, 0, (struct sockaddr *)&dest, sizeof(dest)) != len) {
		fprintf(stderr, "socket4 send: Failed to send ipv4 packet len=%d %s\n", len, strerror(errno));
		return -1;
	}
	return 0;
}

int SocketRaw6::send(u_int8_t* buf, int len, const IPv6Addr& daddr)
{ //cout << "send6 len=" << len << " dest=" << daddr << endl;
	struct sockaddr_in6 dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin6_family = AF_INET6;
	dest.sin6_addr = daddr.getIn6Addr();

	if (sendto(fd_, buf, len, 0, (struct sockaddr *)&dest, sizeof(dest)) != len) {
		fprintf(stderr, "socket6 send: Failed to send ipv6 packet len=%d %s\n", len, strerror(errno));
		return -1;
	}
	return 0;
}
