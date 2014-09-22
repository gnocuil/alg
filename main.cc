#include <cstdio>
#include <iostream>
#include <signal.h>
#include "state.h"
#include "socket.h"
#include "nfqueue.h"

using namespace std;

static void sigHandler(int sig)
{
    exit(0);//TODO: fix the bug
    nfqueue_close();
    exit(0);
}

int main(int argc, char **argv)
{
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
