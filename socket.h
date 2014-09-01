#pragma once

#include "ip.h"

class Socket {
public:
    Socket() : fd_(0) {}
    void init();
    
protected:
    virtual int initSocket() = 0;
    int fd_;
};

class SocketRaw4 : public Socket {
public:
    int send(u_int8_t* buf, int len, const IPv4Addr& daddr);
protected:
    virtual int initSocket();
};

class SocketRaw6 : public Socket {
public:
    int send(u_int8_t* buf, int len, const IPv6Addr& daddr);
protected:
    virtual int initSocket();
};
