#pragma once

#include <string>
#include <netinet/in.h>
#include <ostream>

class IPv4Addr {    
public:
    IPv4Addr();
    IPv4Addr(uint32_t ip);
    IPv4Addr(const std::string& ip);
    
    std::string getString() const;
    uint32_t getInt() const;
    
    friend bool operator< (const IPv4Addr& a, const IPv4Addr& b) {return a.ip_ < b.ip_;}
    friend std::ostream& operator<< (std::ostream& os, const IPv4Addr &ip);
private:
    uint32_t ip_;
};

class IPv6Addr {
public:
    IPv6Addr();
    IPv6Addr(const in6_addr& ip);
    IPv6Addr(const std::string& ip);
    
    std::string getString() const;
    in6_addr getIn6Addr() const;
    
    friend std::ostream& operator<< (std::ostream& os, const IPv6Addr &ip);
    
    friend bool operator< (const IPv6Addr& a, const IPv6Addr& b);
    friend bool operator!= (const IPv6Addr& a, const IPv6Addr& b);

private:
    struct in6_addr ip_;
};

class IP6Port {
public:
    IP6Port();
    IP6Port(const IPv6Addr& ip, uint16_t port);
    
    bool isNull() const;
    
    IPv6Addr getIP() const;
    uint16_t getPort() const;
    
    friend bool operator< (const IP6Port& a, const IP6Port& b);
    
    friend std::ostream& operator<< (std::ostream& os, const IP6Port &ip6Port);
private:
    IPv6Addr ip_;
    uint16_t port_;
};

class IP4Port {
public:
    IP4Port();
    IP4Port(const IPv4Addr& ip, uint16_t port);
    
    bool isNull() const;
    
    IPv4Addr getIP() const;
    uint16_t getPort() const;
    
    friend bool operator< (const IP4Port& a, const IP4Port& b);
    
    friend std::ostream& operator<< (std::ostream& os, const IP4Port &ip4Port);
private:
    IPv4Addr ip_;
    uint16_t port_;
};

