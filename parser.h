#pragma once

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include "communicator.h"

enum DEST { CLIENT, SERVER };

class Parser;
typedef boost::shared_ptr<Parser> ParserPtr;

class Parser {
public:
    Parser(Communicator *c) : c_(c) {add(this, c_);}
    ~Parser();

    void process(const std::string& data);
    
    static void run(Communicator *c);
    static ParserPtr make(std::string protocol);
    
    virtual void run__() = 0;
    
protected:
    Communicator *c_;
    static void add(Parser* p, Communicator *c);
    static std::map<Communicator*, Parser*> mp_;
};


