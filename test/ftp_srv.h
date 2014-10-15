#pragma once
#include <vector>
#include <fcntl.h>
#include <signal.h>
#include <netinet/tcp.h>
class FTPServer {
public:
    void Send(int fd, const std::string& str) {
        send(fd, str.c_str(), str.size(), 0);
    }
    int Recv(int fd) {
        int n = recv(fd, buf, 2000, 0);
        //printf("srv recv %d\n", n);
        buf[n] = 0;
        return n;
    }
    bool startsWith(const string& cmd) {
        return memcmp(buf, cmd.c_str(), cmd.size()) == 0;
    }
    int Connect(IPv4Addr ip, uint16_t port) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd <= 0)  {
            perror("error in socket() #9");
            exit(0);
        }
        struct sockaddr_in serv_addr;
        memset((char *) &serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        serv_addr.sin_addr.s_addr = ip.getInt();
        if (connect(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("ERROR connecting");
            exit(0);
        }
        return fd;
    }
    
    void SendFile(int fd, string filename) {
        printf("SendFile %s\n", filename.c_str());
        FILE *f = fopen(filename.c_str(), "r");
        const int BUF_LEN = 100000;
        char buf[BUF_LEN];
        do {
            int cnt = fread(buf, 1, BUF_LEN, f);
            printf("cnt=%d\n", cnt);
            if (cnt == 0) break;
            send(fd, buf, cnt, 0);
        } while (1);
        fclose(f);
        close(fd);
    }
    
    void process(int fd) {
        Send(fd, "220 (MyALG 0.1)\r\n");
        bool eprt = false;
        string filename;
        int p;
        string ip;
        uint16_t port;
        signal(SIGCHLD,SIG_IGN);
        while (true) {
            int cnt = Recv(fd);
            if (cnt <= 0) break;
            if (startsWith("USER")) {
                Send(fd, "331 Please specify the password.\r\n");
            }
            else if (startsWith("PASS")) {
                Send(fd, "230 Login successful.\r\n");
            }
            else if (startsWith("SYST")) {
                Send(fd, "215 UNIX Type: L8\r\n");
            }
            else if (startsWith("OPTS")) {
                Send(fd, "200 Always in UTF8 mode.\r\n");
            }
            else if (startsWith("PWD")) {
                Send(fd, "257 \"/\"\r\n");
            }
            else if (startsWith("TYPE")) {
                Send(fd, "200 Switching to Binary mode.\r\n");
            }
            else if (startsWith("EPRT")) {
                Send(fd, "200 EPRT command successful. Consider using EPSV.\r\n");
                for (int i = 4; i < cnt; ++i)
                    if (buf[i] == '|') buf[i] = ' ';
                char ip_[100] = {0};
                int port_;
                sscanf(buf + 5, "%d %s %d", &p, ip_, &port_);
                if (p == 1) {
                    ip = ip_;
                    port = (uint16_t)port_;
                } else {
                    break;
                }
            
                eprt = true;
            }
            else if (startsWith("RETR")) {
                filename = string(buf + 5, buf + cnt - 2);
//                puts(filename.c_str());
                int fd2 = 0;
                if (p == 1)
                    fd2 = Connect(IPv4Addr(ip), port);
                Send(fd, "150 Opening BINARY mode data connection for " + filename + " (? bytes).\r\n");
                SendFile(fd2, filename);
                Send(fd, "226 Transfer complete.\r\n");
                break;
            } else {
                printf("unknown %s", buf);
                break;
            }
        }
//        puts("quit!");
        close(fd);
    }
    void run() {
        int ret = pthread_create(&tid, NULL, _thread_t, this);
    }
    static void* _thread_t(void* param) {
        FTPServer* test = (FTPServer*)param;
        test->run__();
        return NULL;
    }
    void run__() {
        int fd = socket(AF_INET6, SOCK_STREAM, 0);
        if (fd <= 0)  {
            perror("error in socket() #9");
            exit(0);
        }
        int on = 0;
        setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));
//        on = 1;
//        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
        
        struct sockaddr_in6 serv_addr;
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin6_family = AF_INET6;
        serv_addr.sin6_addr = in6addr_any;
        serv_addr.sin6_port = htons(21);
        if (bind(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0)  {
            perror("error in bind()");
            exit(0);
        }

        if (listen(fd, 10000) != 0)  {
            perror("error in listen()");
            exit(0);
        }
        
        while (true) {
            struct sockaddr_in6 cli_addr;
            socklen_t clilen = sizeof(cli_addr);
            int newdatafd = accept(fd, (struct sockaddr *) &cli_addr, &clilen);
//            int on = 1;
//            setsockopt(newdatafd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
            int pid;
            if ((pid = fork()) == 0) {
                process(newdatafd);
            } else son.push_back(pid);
        }

    }
    vector<int> son;
private:
    char buf[2001];
    pthread_t tid;
};
