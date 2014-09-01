#pragma once

#include <stdint.h>
#include <boost/shared_ptr.hpp>

#include <cstdio>

const int BUF_LEN = 2000;

class Packet {
public:
    
    void setIbufLen(int len) { ibuf_len_ = len; }
    
    uint8_t** getIbuf() { return (uint8_t**)(&ibuf_); }
    
    void print() {
        for (int i = 0; i < ibuf_len_; ++i) {
            printf("%02x ", ibuf_[i] & 0xff);
            if (i % 16 == 15) printf("\n");
        }
        printf("\n");
            
    }
    
    u_int16_t hw_protocol;
    u_int32_t id;
    
//private:
    uint8_t *ibuf_;
    int ibuf_len_;
    
    uint8_t obuf_[BUF_LEN];
};

typedef boost::shared_ptr<Packet> PacketPtr;
