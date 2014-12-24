#include <sstream>
#include <iostream>
#include <string>
#include <cstring>
#include "flow.h"
#include "state.h"
#include "packet.h"

using namespace std;

std::ostream& operator<< (std::ostream& os, const Flow &f)
{
    return os << "Flow(" << f.ip6p << "<->" << f.ip6srv << " <=====> " << f.ip4p << "<->" << f.ip4srv << ")";
}

std::ostream& operator<< (std::ostream& os, const FlowPtr &f)
{
    if (!f) return os << "Flow(NULL)";
    return os << *f;
}

ParserPtr Flow::getParser(std::string protocol, DEST dest)
{
    if (dest == CLIENT) {
        switch (sm.protocols[protocol].ptype_s2c) {
        case StateManager::STATELESS:
            return Parser::make(protocol, new StatelessCommunicator(), this);
        case StateManager::STATEFUL:
            if (!parsers_s2c.count(protocol) || !parsers_s2c[protocol] || parsers_s2c[protocol]->end) {
                parsers_s2c[protocol] = Parser::make(protocol, new StatefulCommunicator(), this);
            }
            return parsers_s2c[protocol];
        default://NONE
            ;
        }
    } else {
        switch (sm.protocols[protocol].ptype_c2s) {
        case StateManager::STATELESS:
            return Parser::make(protocol, new StatelessCommunicator(), this);
        case StateManager::STATEFUL:
            if (!parsers_c2s.count(protocol) || !parsers_c2s[protocol] || parsers_c2s[protocol]->end) {
                parsers_c2s[protocol] = Parser::make(protocol, new StatefulCommunicator(), this);
            }
            return parsers_c2s[protocol];
        default://NONE
            ;
        }
    }
    return ParserPtr();
}

int Flow::getOffset(DEST dest)
{
    if (dest == SERVER)
        return offset_c2s;
    else
        return offset_s2c;
}

void Flow::addOffset(DEST dest, int delta)
{
    if (dest == SERVER)
        offset_c2s += delta;
    else
        offset_s2c += delta;
}

void Flow::save(std::string content)
{
    if (!fout) {
        ostringstream sos;
        sos << "Flow(" << ip6p << "-" << ip6srv << "<->" << ip4p << "-" << ip4srv << ")";
        fout = fopen(sos.str().c_str(), "w");
    }
    fprintf(fout,"\n-----------len=%d---------------\n", (int)content.size());    
    for (int i = 0; i < content.size(); ++i)
        fprintf(fout, "%c", content[i]);
}

int Flow::getPktOffset(PacketPtr pkt) 
{
    if (!pkt->isTCP()) return 0;
    DEST dest = pkt->getDEST();
    tcphdr* tcp = pkt->getTCPHeader();
    if (!init_seq_set[dest]) {
        init_seq_set[dest] = true;
        init_seq[dest] = ntohl(tcp->seq);
        return 0;
    } else {
        uint32_t seq = ntohl(tcp->seq);
        seq = seq - init_seq[dest];
        return (int)seq;
    }
}

int Flow::modify(PacketPtr pkt, std::vector<Operation>& ops, ParserPtr parser)
{
    if (ops.size() == 0)
        return 0;
    DEST dest = pkt->getDEST();
    sort(ops.begin(), ops.end());//sort ?
    int len = pkt->getTransportLen();
	int hl = pkt->getTransportHeaderLen();
	len -= hl;
	char *d_, *d, *s;
	if (pkt->isTCP()) {
	    d = (char*)pkt->getTCPHeader() + hl;
    	s = (char*)pkt->getIbufTCPHeader() + hl;
    } else if (pkt->isUDP()) {
	    d = (char*)pkt->getUDPHeader() + hl;
    	s = (char*)pkt->getIbufUDPHeader() + hl;
    } else {
        return 0;
    }
    d_ = d;
	int len_old = len;
	
	//get offset
	int offset = 0;
	if (pkt->isTCP()) {
	    offset = getPktOffset(pkt);
	    //printf("Packet Offset = %d\n", offset);
	}
	/*
	int tl = 0;//tl===trail[dest].size()
	if (trail[dest].size() > 0) {printf("found tl %s\n", trail[dest].c_str());
	    tl = trail[dest].size();
	    memcpy(d, trail[dest].c_str(), tl);
	    d += tl;
	    trail[dest] = "";
	}*/
	for (int i = 0; i < ops.size(); ++i) {
	    //printf("replace : %d %d [", ops[i].start_pos, ops[i].end_pos);
	    //for (int j = ops[i].start_pos; j < ops[i].end_pos; ++j)
	    //    putchar(s[j - offset]);
	    //printf("] with <%s>\n", ops[i].newdata.c_str());
	    int delta = ops[i].newdata.size() - (ops[i].end_pos - ops[i].start_pos);
	    len += delta;
	}
	//printf("newlen=%d\n", len);
	int cnt = 0, pd, ps;
	bool inside = false;
	for (pd = ps = 0; pd < len; ++pd) {
	    if (!inside && ps == ops[cnt].start_pos - offset) {
	        if (ops[cnt].newdata.size() == 0) {
	            ps = ops[cnt++].end_pos - offset;
	        } else {
    	        inside = true;
	            ps = 0;
	        }
	    }
	    if (inside) {
	        d[pd] = ops[cnt].newdata[ps++];
	        if (ps == ops[cnt].newdata.size()) {
	            inside = false;
	            ps = ops[cnt++].end_pos - offset;
	        }
	    } else {
	        d[pd] = s[ps++];
	    }
	}
	
	//d -= tl;
	//len += tl;
	
	int maxPos = parser->maxPos;//check max length
	//cout << "modify(): maxPos_1="<<maxPos<<endl;
	if (maxPos > 0) {
	    for (int i = 0; i < ops.size(); ++i) {
	        if (ops[i].end_pos <= parser->maxPos) {
        	    int delta = ops[i].newdata.size() - (ops[i].end_pos - ops[i].start_pos);
	            maxPos += delta;
	            parser->maxPos += delta;
	        }
	    }
	    maxPos += parser->maxLen;
	}
	//cout << "modify(): maxPos_2="<<maxPos<<endl;
	maxPos -= offset;
	if (maxPos > 0 && maxPos <= len) {//TODO: absolute offset
	    //cout << "translate: len=" << len << " maxPos=" << maxPos << " " << "offs="<<offset<< endl;
	    string oldcontent = string(d + maxPos, len - maxPos);
	    string newcontent = parser->getContentLengthExceed(oldcontent);
	    if (newcontent != oldcontent) {
	        memcpy(d + maxPos, newcontent.c_str(), newcontent.size());
	        len = len - oldcontent.size() + newcontent.size();
	    }
	}

	if (pkt->isTCP() && len > len_old && len_old > 1400) {//TODO: exceed MSS)
	    //cout << "Exceed MSS: " << len << " > " << len_old<<endl;
	    if (sm.extra == SHIFT) {
	        trail_shift[pkt->nextSeq()] = string(d_ + len_old, d_ + len);
	    } else {
    	    trail[dest] = string(d_ + len_old, d_ + len);
    	}
	    //cout << "trail=" << trail[dest] << endl;
        len = len_old;
        //TODO: last packet
	}
	
	pkt->setTransportLen(len + hl);
	return len - len_old;
}

void Flow::shift(PacketPtr pkt, char* d_, int len) {
    uint32_t seq = pkt->curSeq();
	if (sm.extra == SHIFT && trail_shift[seq].size() > 0) {//printf("shift len=%d\n", len);
	    int slen = trail_shift[seq].size();
	    //printf("In SHIFT: slen=%d %s\n", slen, trail[dest].c_str());
	    string newt = string(d_ + len - slen, d_ + len);
	    //printf("newt=%s\n", newt.c_str());
	    for (int i = len - 1; i >= slen; --i)
	        d_[i] = d_[i - slen];
	    memcpy(d_, trail_shift[seq].c_str(), slen);
	    trail_shift[pkt->nextSeq()] = newt;
	}
}

void Flow::count(PacketPtr pkt, DEST dest)
{
    if (!pkt->isTCP()) return;
    tcphdr* tcp = pkt->getIbufTCPHeader();
    if (segments[dest].size() == 0) {
        Segment s;
        s.seq = ntohl(tcp->seq);
        s.len = pkt->getTransportLen() - pkt->getTransportHeaderLen();
        segments[dest].push_back(s);
        //cout << "!!!!!!!!new segment: " << s.seq << " " << s.len << std::endl;
        //cout << "\t" << pkt->getTransportLen() << " " << pkt->getTransportHeaderLen() << endl;
    } else {
        Segment s;
        s.seq = ntohl(tcp->seq);
        s.len = pkt->getTransportLen() - pkt->getTransportHeaderLen();
        uint32_t seq_exp = (segments[dest][segments[dest].size() - 1].seq + segments[dest][segments[dest].size() - 1].len);
        if (s.seq == seq_exp) {
            segments[dest].push_back(s);
        } else {//seq error!
        }
        //cout << "!!!!!!!!new segment: " << s.seq << " " << s.len << " " << seq_exp << std::endl;
    }
}

