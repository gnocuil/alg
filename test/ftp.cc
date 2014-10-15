#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "ftp.h"
#include "../ip.h"
#include "ftp_srv.h"

using namespace std;

const int TESTS = 4;
const int REPEAT_TIMES = 10;

class FTPTests {
public:
    FTPTests(std::string server_ip) {
        for (int i = 0; i < TESTS; ++i) {
            tests[i] = new FTPTest(server_ip);
        }
    }
    void run() {
        for (int i = 0; i < TESTS; ++i)
            tests[i]->run();
        do {
            sleep(1);
            bool ok = true;
            for (int i = 0; i < TESTS; ++i)
                if (tests[i]->t2 == 0) {
                    ok = false;
                    break;
                }
            if (ok) break;
        } while (1);
        long long min_t1 = tests[0]->t1;
        long long max_t1 = tests[0]->t1;
        long long max_t2 = tests[0]->t2;
        long long recvbytes = 0;
        long long sendbytes = 0;
        time_eprt = 0;
        time_pwd = 0;
        for (int i = 0; i < TESTS; ++i) {
            printf("%d\t%lld\t%lld\n", i, tests[i]->t1, tests[i]->t2);
            if (tests[i]->t1 < min_t1) min_t1 = tests[i]->t1;
            if (tests[i]->t1 > max_t1) max_t1 = tests[i]->t1;
            if (tests[i]->t2 < max_t2) max_t2 = tests[i]->t2;
            sendbytes += tests[i]->sendbytes;
            recvbytes += tests[i]->recvbytes;
            time_eprt += tests[i]->time_eprt;
            time_pwd += tests[i]->time_pwd;
            cout<<"eprt time " << tests[i]->time_eprt/1000<<"\n";
            cout<<"pwd  time " << tests[i]->time_eprt/1000<<"\n";
        }
        time_total = max_t2 - min_t1;
        cout << "total=" << time_total / 1000 << "us\n";
        cout << "total=" << (max_t1 - min_t1) / 1000 << "us\n";
        cout << "s/r " << sendbytes << " " << recvbytes << endl;
        recv_KBps = recvbytes  * 1000000000 / 1024 / time_total;
        cout << "s/r KB/s " << sendbytes * 1000000000 / 1024 / time_total << " " << recv_KBps << endl;
        cout << "time_eprt=" << time_eprt / 1000 << "us\n";
        cout << "time_pwd=" << time_pwd / 1000 << "us\n";
        
        time_total /= 1000;
        time_pwd /= 1000 * TESTS;
        time_eprt /= 1000 * TESTS;
    }
//private:
    FTPTest* tests[TESTS];
    long long time_total;
    long long time_pwd;
    long long time_eprt;
    long long recv_KBps;
};

int main(int argc, char **argv)
{
    FTPServer srv;
    if (argc > 1) {
        char tmp[100];
        if (inet_pton(AF_INET6, argv[1], tmp) == 1) {

            srv.run();
            FTPTests* tests[REPEAT_TIMES];
            for (int i = 0; i < REPEAT_TIMES; ++i) {
                tests[i] = new FTPTests(argv[1]);
                tests[i]->run();
                sleep(1);
            }
            for (int i = 0; i < REPEAT_TIMES; ++i) {
                cout << "#" << i << "\t" << tests[i]->time_total << "\t" << tests[i]->recv_KBps << "\t" << tests[i]->time_eprt << "\t" << tests[i]->time_pwd << endl;
            }
            
        } else if (inet_pton(AF_INET, argv[1], tmp) == 1) {
            puts("IPv4 addr! quit...");
            return 0;
        } else {
            puts("invalid addr! quit...");
            return 0;
        }
    }
//    for (int i = 0; i < srv.son.size(); ++i)
//        kill(srv.son[i], SIGKILL);
//    exit(0);
}
