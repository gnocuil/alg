#include "parser.h"
#include "packet.h"
#include "http/HTTPParser.h"
#include "ftp/FTPParser.h"
#include "sip/SIPParser.h"
#include "parser_make.h"

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

void Parser::addOperation(int startPos, int endPos, Operation::OP operation, const std::string newdata, bool count) {
    Operation op;
    op.start_pos = startPos;
    op.end_pos = endPos;
    op.op = operation;
    op.newdata = newdata;
    op.cnt = count;
    addOperation(op);
}

void Parser::addOperation(TokenType token, Operation::OP operation, const std::string newdata, bool count) {
    addOperation(token.pos, token.pos_end, operation, newdata, count);
}

int Parser::totalDelta() const {
    int delta = 0;
    for (int i = 0; i < ops.size(); ++i) if (ops[i].cnt) {
        delta += ops[i].newdata.size() - (ops[i].end_pos - ops[i].start_pos);
    }
    return delta;
}

bool operator<(const Operation& a, const Operation& b) {
    return a.start_pos < b.start_pos;
}
