#include "parser.h"
#include "http/HTTPParser.h"

std::map<Communicator*, Parser*> Parser::mp_;

Parser::~Parser() {
    if (c_) delete c_;
}

void Parser::add(Parser* p, Communicator *c) {
    mp_[c] = p;
}

void Parser::run(Communicator *c) {
    Parser *p = mp_[c];
    if (p) {c->test(123);
        p->run__();
    }
}

ParserPtr Parser::make(std::string protocol) {
    if (protocol == "http") {
        ParserPtr p = ParserPtr(new HTTPParser(new StatefulCommunicator()));
        return p;
    }
    return ParserPtr();
}

void Parser::process(const std::string& data) {
    c_->addData(data);
}
