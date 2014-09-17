#pragma once

#include <string>
#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "communicator.h"

enum DEST { CLIENT, SERVER };

class TokenType;

class Operation {
public:
    enum OP {
        REPLACE
    };
    int start_pos;
    int end_pos;//replace data[start_pos,end_pos) with new data
    OP op;
    std::string newdata;
    bool cnt;
};
bool operator<(const Operation& a, const Operation& b);

class Parser;
typedef boost::shared_ptr<Parser> ParserPtr;

class Parser {
public:
    Parser(Communicator *c) : c_(c) {add(this, c_);}
    ~Parser();

    std::vector<Operation> process(const std::string& data);
    
    static void run(Communicator *c);
    static ParserPtr make(std::string protocol);
    
    virtual void run__() = 0;
    
    void addOperation(const Operation& op);
    void addOperation(int startPos, int endPos, Operation::OP operation, const std::string newdata, bool count = false);
    void addOperation(TokenType token, Operation::OP operation, const std::string newdata, bool count = false);
    
    int totalDelta() const;
    
protected:
    Communicator *c_;
    static void add(Parser* p, Communicator *c);
    static std::map<Communicator*, Parser*> mp_;
    
    std::vector<Operation> ops;
};


