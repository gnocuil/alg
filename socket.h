#pragma once

#include "ip.h"

class Socket {
public:
    Socket() : fd_(0)/*,  smutex(PTHREAD_MUTEX_INITIALIZER)*/ {}
    void init();
    
//    void lock() {pthread_mutex_lock(&smutex);}
//    void unlock() {pthread_mutex_unlock(&smutex);}
protected:
    virtual int initSocket() = 0;
    int fd_;
//    pthread_mutex_t smutex;
};

class SocketRaw4 : public Socket {
public:
    int send(u_int8_t* buf, int len, const IPv4Addr& daddr);
protected:
    virtual int initSocket();
};

class SocketRaw6 : public Socket {
public:
    int send(u_int8_t* buf, int len, const IPv6Addr& daddr, bool keep = false);
    int timeout();
protected:
    virtual int initSocket();
    u_int8_t buf_[2000];
    int len_;
    IPv6Addr daddr_;
};

//extern SocketRaw4 socket4;
//extern SocketRaw6 socket6;

void init_socket();
