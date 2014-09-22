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
    Operation()
        : cnt(false)
    {}
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

class Flow;

class Parser;
typedef boost::shared_ptr<Parser> ParserPtr;

class Parser {
public:
    Parser(Communicator *c)
        : c_(c),
          endPos(-1),
          maxPos(-1),
          end(false) {
        add(this, c_);
    }
    ~Parser();

    std::vector<Operation> process(const std::string& data);
    
    static void run(Communicator *c);
    static ParserPtr make(std::string protocol, Communicator *c, Flow* flow_);
    static Parser* getParser(Communicator *c);
    
    virtual void run__() = 0;
    
    void addOperation(const Operation& op);
    void addOperation(int startPos, int endPos, Operation::OP operation, const std::string newdata, bool count = false);
    void addOperation(TokenType token, Operation::OP operation, const std::string newdata, bool count = false);
    
    int totalDelta() const;
    
    int endPos;
    bool end;
    int maxPos, maxLen;
    
    virtual std::string getContentLengthExceed(std::string content) {return content;}
    
protected:
    void setLengthEnd(int pos, int len);
    
    void setLengthMax(int pos, int len);
    
    Communicator *c_;
    static void add(Parser* p, Communicator *c);
    static std::map<Communicator*, Parser*> mp_;
    
    std::vector<Operation> ops;
    Flow* flow;
};


