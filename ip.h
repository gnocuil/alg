#pragma once

#include <netinet/in.h>

class IPv4Addr {
public:
    uint32_t ip;
};

class IPv6Addr {
public:
    struct in6_addr ip;
};

class IP6Port {
private:
    IPv6Addr ip_;
    uint16_t port_;
};

class IP4Port {
    IP4Port(IPv4Addr ip, uint16_t port);
    
private:
    IPv4Addr ip_;
    uint16_t port_;
};
