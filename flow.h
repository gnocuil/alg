#pragma once
#include <string>
#include <boost/shared_ptr.hpp>
#include "ip.h"
#include "parser.h"

class Packet;

class Flow {
public:
    Flow()
        : offset_c2s(0),
          offset_s2c(0),
          ignore_(false),
          protocol(""),
          fout(NULL),
          tid(-1),
          analyze(0)
    { init_seq_set[0] = init_seq_set[1] = false; trail[0] = trail[1] = "";
    }
    Flow(const IP6Port& ip6p_, const IP4Port& ip4p_, const IPv6Addr& ip6srv_, const IPv4Addr& ip4srv_)
        : ip6p(ip6p_),
          ip4p(ip4p_),
          ip6srv(ip6srv_),
          ip4srv(ip4srv_),
          offset_c2s(0),
          offset_s2c(0),
          ignore_(false),
          protocol(""),
          fout(NULL),
          tid(-1),
          analyze(0)
    { init_seq_set[0] = init_seq_set[1] = false; trail[0] = trail[1] = "";
    }
    
    //ip4p1: dest(as server); ip4p2: source (as client);
    Flow(const IP4Port& ip4p1_, const IP4Port& ip4p2_)
        : offset_c2s(0),
          offset_s2c(0),
          ip4p(ip4p1_),
          ignore_(false),
          protocol(""),
          fout(NULL),
          tid(-1),
          analyze(0)
    { init_seq_set[0] = init_seq_set[1] = false; trail[0] = trail[1] = "";
    }
          
    ~Flow() {
        //std::cout << "Delete " << *this << std::endl;
    }
    friend std::ostream& operator<< (std::ostream& os, const Flow &f);
    int getOffset(DEST dest);
    void addOffset(DEST dest, int delta);
    
    ParserPtr getParser(std::string protocol, DEST dest);
    
    void setIgnore() {ignore_ = true;}
    bool ignored() {return ignore_;}
    
    void setProtocol(std::string protocol_) {protocol = protocol_;}
    std::string getProtocol() const {return protocol;}
    
    void save(std::string content);
    
    int modify(boost::shared_ptr<Packet> pkt, std::vector<Operation>& ops, ParserPtr parser);
    
    void count(boost::shared_ptr<Packet> pkt, DEST dest);
    
    void shift(boost::shared_ptr<Packet> pkt, char* d_, int len);
    
//private:
    IP6Port ip6p;
    IPv6Addr ip6srv;
    IP4Port ip4p;
    IPv4Addr ip4srv;
    
    std::string trail[2];
    std::map<uint32_t, std::string> trail_shift;
    
    int tid;
    
    int analyze;

private:

    int getPktOffset(boost::shared_ptr<Packet> pkt);

    int offset_c2s;
    int offset_s2c;
    ParserPtr https2c;
    bool ignore_;
    std::string protocol;
    std::map<std::string, ParserPtr> parsers_c2s;
    std::map<std::string, ParserPtr> parsers_s2c;
    
    class Segment {
    public:
        uint32_t seq;
        uint32_t len;
    };
    
    std::vector<Segment> segments[2];
    FILE* fout;
    
    bool init_seq_set[2];
    int  init_seq[2];
    
    
};

typedef boost::shared_ptr<Flow> FlowPtr;
std::ostream& operator<< (std::ostream& os, const FlowPtr &f);

