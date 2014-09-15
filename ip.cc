#include <cstring>
#include <sstream>
#include <arpa/inet.h>
#include "ip.h"

using namespace std;

IPv4Addr::IPv4Addr()
    : ip_(0)
{
}

IPv4Addr::IPv4Addr(uint32_t ip)
    : ip_(ip)
{
}

IPv4Addr::IPv4Addr(const std::string& ip)
{
    inet_pton(AF_INET, ip.c_str(), &ip_);
}

std::string IPv4Addr::getString() const
{
    char buf[INET_ADDRSTRLEN + 1] = {0};
    inet_ntop(AF_INET, &ip_, buf, INET_ADDRSTRLEN);
    return buf;
}

uint32_t IPv4Addr::getInt() const
{
    return ip_;
}

ostream& operator<< (ostream& os, const IPv4Addr &ip)
{
    return os << ip.getString();
}

IPv6Addr::IPv6Addr()
{
    memset(ip_.s6_addr, 0, 16);
}

IPv6Addr::IPv6Addr(const in6_addr& ip)
    : ip_(ip)
{
}

IPv6Addr::IPv6Addr(const std::string& ip)
{
    inet_pton(AF_INET6, ip.c_str(), &ip_);
}

std::string IPv6Addr::getString() const
{
    char buf[INET6_ADDRSTRLEN + 1] = {0};
    inet_ntop(AF_INET6, &ip_, buf, INET6_ADDRSTRLEN);
    return buf;
}

in6_addr IPv6Addr::getIn6Addr() const
{
    return ip_;
}

ostream& operator<< (ostream& os, const IPv6Addr &ip)
{
    return os << ip.getString();
}

bool operator<(const IPv6Addr& a, const IPv6Addr& b)
{
    if (a.ip_.s6_addr32[0] != a.ip_.s6_addr32[0])
        return a.ip_.s6_addr32[0] < a.ip_.s6_addr32[0];
    if (a.ip_.s6_addr32[1] != a.ip_.s6_addr32[1])
        return a.ip_.s6_addr32[1] < a.ip_.s6_addr32[1];
    if (a.ip_.s6_addr32[2] != a.ip_.s6_addr32[2])
        return a.ip_.s6_addr32[2] < a.ip_.s6_addr32[2];
    return a.ip_.s6_addr32[3] < a.ip_.s6_addr32[3];
}

bool operator!=(const IPv6Addr& a, const IPv6Addr& b)
{
    return (a.ip_.s6_addr32[0] != a.ip_.s6_addr32[0]) ||
           (a.ip_.s6_addr32[1] != a.ip_.s6_addr32[1]) ||
           (a.ip_.s6_addr32[2] != a.ip_.s6_addr32[2]) ||
           (a.ip_.s6_addr32[3] != a.ip_.s6_addr32[3]);
}

IP4Port::IP4Port()
    : ip_(0),
      port_(0)
{
}

IP4Port::IP4Port(const IPv4Addr& ip, uint16_t port)
    : ip_(ip),
      port_(port)
{    
}

bool IP4Port::isNull() const
{
    return port_ == 0;
}

IPv4Addr IP4Port::getIP() const
{
    return ip_;
}

uint16_t IP4Port::getPort() const
{
    return port_;
}

std::string IP4Port::getPortString() const
{
    char buf[10] = {0};
    sprintf(buf, "%d", ntohs(port_));
    return buf;
}

ostream& operator<< (ostream& os, const IP4Port &ip4Port)
{
    return os << ip4Port.ip_ << ":" << ntohs(ip4Port.port_);
}

ostream& operator<< (ostream& os, const IP6Port &ip6Port)
{
    return os << ip6Port.ip_ << ":" << ntohs(ip6Port.port_);
}

IP6Port::IP6Port()
    : ip_("::"),
      port_(0)
{
}

IP6Port::IP6Port(const IPv6Addr& ip, uint16_t port)
    : ip_(ip),
      port_(port)
{
}

IP6Port::IP6Port(const IPv6Addr& ip, std::string port)
    : ip_(ip)
{
    istringstream sin(port);
    sin >> port_;
    port_ = ntohs(port_);
}

bool IP6Port::isNull() const
{
    return port_ == 0;
}

IPv6Addr IP6Port::getIP() const
{
    return ip_;
}

uint16_t IP6Port::getPort() const
{
    return port_;
}

bool operator< (const IP4Port& a, const IP4Port& b)
{
    if (a.ip_.getInt() != b.ip_.getInt())
        return a.ip_.getInt() < b.ip_.getInt();
    return a.port_ < b.port_;
}

bool operator< (const IP6Port& a, const IP6Port& b)
{
    if (a.ip_ != b.ip_)
        return a.ip_ < b.ip_;
    return a.port_ < b.port_;
}

