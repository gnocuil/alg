#include "parser.h"
#include "packet.h"
#include "http/HTTPParser.h"
#include "ftp/FTPParser.h"

using namespace std;

std::map<Communicator*, Parser*> Parser::mp_;

Parser::~Parser() {
    if (c_) delete c_;
}

void Parser::add(Parser* p, Communicator *c) {
    mp_[c] = p;
}

void Parser::run(Communicator *c) {
    Parser *p = mp_[c];
    if (p) {
        p->run__();
    }
}

ParserPtr Parser::make(std::string protocol) {
    if (protocol == "http") {
        ParserPtr p = ParserPtr(new HTTPParser(new StatefulCommunicator()));
        return p;
    } else if (protocol == "ftp") {
        ParserPtr p = ParserPtr(new FTPParser(new StatelessCommunicator()));
        return p;
    }
    return ParserPtr();
}

std::vector<Operation> Parser::process(const std::string& data) {
    c_->addData(data);
    std::vector<Operation> ret = ops;
    ops.clear();
    return ret;
}

void Parser::addOperation(const Operation& op) {
    switch (op.op) {
    case Operation::REPLACE:
        cout << "addOperation: repalce[" << op.start_pos << "," << op.end_pos << ") with '" << op.newdata << "'\n";
        ops.push_back(op);
    default:
        break;
    }
}

void Parser::addOperation(int startPos, int endPos, Operation::OP operation, const std::string newdata)
{
    Operation op;
    op.start_pos = startPos;
    op.end_pos = endPos;
    op.op = operation;
    op.newdata = newdata;
    addOperation(op);
}

bool operator<(const Operation& a, const Operation& b) {
    return a.start_pos < b.start_pos;
}
