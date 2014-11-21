#pragma once
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

#include "../ip.h"

using namespace std;

class FTPTest {
public:
    FTPTest(std::string server_ip)
        : serverip(server_ip),
          t2(0),
          sendbytes(0),
          recvbytes(0) {
    }
    void run() {
        int ret = pthread_create(&tid, NULL, _thread_t, this);
    }
    static void* _thread_t(void* param) {
        FTPTest* test = (FTPTest*)param;
        test->run__();
        return NULL;
    }
    void Send(const std::string& str) {
        sendbytes += str.size();
        send(fd, str.c_str(), str.size(), 0);
    }
    int Recv(const std::string& str) {
        int n = recv(fd, buf, 2000, 0);
        recvbytes += n;
        buf[n] = 0;
        
        int ret = 0;
        if (n >= str.size()) {
            if (memcmp(buf, str.c_str(), str.size()) != 0)
                ret = -1;
        } else
            ret = -1;
//        if (ret != 0)
//            printf("Recv() expect %s got %s", str.c_str(), buf);
//        printf("recv ret=%d %s\n", ret,buf);
        return ret;
    }
    int Recv_End(const std::string& str) {
        int n = recv(fd, buf, 2000, 0);
        recvbytes += n;
        buf[n] = 0;
        
        int ret = 0;
        if (n >= str.size()) {
            if (memcmp(buf + n - str.size(), str.c_str(), str.size()) != 0)
                ret = -1;
        } else
            ret = -1;
//        if (ret != 0)
//            printf("Recv_End() expect %s got %s", str.c_str(), buf);
//        printf("recv ret=%d %s\n", ret,buf);
        return ret;
    }
    
    int Recv_Data() {
        int n = recv(newdatafd, buf, 20000, 0);
        recvbytes += n;
        
//        printf("recv data n=%d \n", n);
        return n;
    }
    void run__() {
        static int _pid = 0;
        int pid = _pid++;
        cout << "Run tid " << pid << endl;
        t1 = time();
        
        fd = socket(PF_INET6, SOCK_STREAM, 0);
        if (fd <= 0) {
            perror("error in socket() #86");
            exit(0);
        }
        struct sockaddr_in6 serv_addr;
        memset((char *) &serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin6_family = AF_INET6;
        serv_addr.sin6_port = htons(21);
        serv_addr.sin6_addr = serverip.getIn6Addr();
        if (connect(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("ERROR connecting");
            exit(0);
        }
        Recv("220");
        Send("USER anonymous\r\n");
        Recv("331");
        Send("PASS anon@localhost\r\n");
        Recv("230");
        Send("SYST\r\n");
        Recv("215");
/*
        Send("FEAT\r\n");
        while (Recv_End("211 End\r\n") == -1)
            ;
cout << "Run tid " << tid << " 211 END" << endl;
*/
        Send("OPTS UTF8 ON\r\n");
        Recv("200");
        long long t1_pwd = time();
        Send("PWD\r\n");
        Recv("257");
        time_pwd = time() - t1_pwd;
        Send("TYPE I\r\n");
        Recv("200");
        
        datafd = socket(AF_INET6, SOCK_STREAM, 0);
        if (datafd <= 0)  {
            perror("error in socket() #118");
            exit(0);
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin6_family = AF_INET6;
        serv_addr.sin6_addr = in6addr_any;
//        serv_addr.sin6_port = htons(portno);
        if (bind(datafd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0)  {
            perror("error in bind()");
            exit(0);
        }
        struct sockaddr_in6 sin;
        socklen_t len = sizeof(sin);
        IPv6Addr myaddr;
        uint16_t port;
        if (getsockname(fd, (struct sockaddr *)&sin, &len) == -1) {
            perror("getsockname");
            exit(0);
        } else {
            myaddr = IPv6Addr(sin.sin6_addr);
        }

        if (getsockname(datafd, (struct sockaddr *)&sin, &len) == -1) {
            perror("getsockname");
            exit(0);
        } else {
            port = ntohs(sin.sin6_port);
        }
        sprintf(bufs, "EPRT |2|%s|%d|\r\n", myaddr.getString().c_str(), port);
        //cout << "CMD tid " << pid << " " << bufs;
        long long t1_eprt = time();
        for (int i = 0; i < 1; ++i) {
        Send(bufs);
        Recv("200");
        }
        time_eprt = time() - t1_eprt;
/*        
//        Send("RETR test_small.txt\r\n");
//        Send("RETR 1MB.bin\r\n");
        Send("RETR 1KB.bin\r\n");
        
        //static int cnt1=0;cout << "After Send tid " << pid << "  cnt="<<++cnt1<<endl;
        
        if (listen(datafd, 1) != 0)  {
            perror("error in listen()");
            exit(0);
        }
        //static int cnt2=0;cout << "After Listen tid " << pid << "  cnt="<<++cnt2<<endl;
        struct sockaddr_in6 cli_addr;
        socklen_t clilen;
        clilen = sizeof(cli_addr);
        newdatafd = accept(datafd, (struct sockaddr *) &cli_addr, &clilen);
        Recv("150");
//        static int cnt3=0;cout << "After Accept tid " << pid << "  cnt="<<++cnt3<<endl;
        do {
            if (Recv_Data() <= 0)
                break;
        } while (1);
        close(newdatafd);
//        static int cnt4=0;cout << "After Recv tid " << pid << "  cnt="<<++cnt4<<endl;
        Recv("226");
*/
        shutdown(fd, SHUT_RDWR);
        close(fd);
        
        
        t2 = time();
        cout << "End tid " << pid << endl;
//        cout << (t2 - t1) / 1000 << "us" << endl;
    }
    static long long time() {
        struct timespec t;
        memset(&t, 0, sizeof(t));
        clock_gettime(CLOCK_REALTIME, &t);
        return t.tv_sec * 1000000000 + t.tv_nsec;
    }
//private:
    IPv6Addr serverip;
    pthread_t tid;
    int fd;
    int datafd;
    int newdatafd;
    char bufs[2000];
    char buf[20000];
    long long t1;
    long long t2;
    
    long long sendbytes;
    long long recvbytes;
    long long time_eprt;
    long long time_pwd;
};
