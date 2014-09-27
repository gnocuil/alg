// Generated by Bisonc++ V4.05.00 on Fri, 05 Sep 2014 00:16:25 +0800

#ifndef HTTPParser_h_included
#define HTTPParser_h_included

// $insert baseclass
#include "HTTPParserbase.h"
// $insert scanner.h
#include "HTTPScanner.h"

#include "../parser.h"

#undef HTTPParser
class HTTPParser: public HTTPParserBase, public Parser
{
    // $insert scannerobject
    HTTPScanner d_scanner;
        
    public:
        HTTPParser(Communicator *c)
          : Parser(c),
            d_scanner(c->getAddrIStream()) {
            d_scanner.setSval(&d_val__);
        }
        int parse();
        
        virtual void run__() {puts("http run__!");
            parse();
        }
        virtual std::string getContentLengthExceed(std::string content) {
            //std::cout << "http::getCLE:" << content << std::endl;
            std::ostringstream os;
            if (content.size() > 0) {
                os << "\r\n" << std::hex << content.size() << "\r\n" << content << "\r\n0\r\n\r\n";
            } else {
                os << "\r\n0\r\n\r\n";
            }
            //std::cout<<"http::getCLE result:"<<os.str()<<std::endl;
            return os.str();
        }
        
        virtual int getCount() { d_scanner.getCount(); }

    private:
        void error(char const *msg);    // called on (syntax) errors
        int lex();                      // returns the next token from the
                                        // lexical scanner. 
        void print();                   // use, e.g., d_token, d_loc

    // support functions for parse():
        void executeAction(int ruleNr);
        void errorRecovery();
        int lookup(bool recovery);
        void nextToken();
        void print__();
        void exceptionHandler__(std::exception const &exc);
        std::string curTag;
        int ignore;
        void beginTag(HTTPParserBase::STYPE__ tag) {
            if (ignore) return;
            curTag = tag.s;
            //std::cout << "begin tag:[" << tag << "] pos=" << tag.pos << " " << tag.pos_end << std::endl;
            if (tag.s == "textarea" || tag.s == "script") {
                ++ignore;
            }
        }
        void endTag(HTTPParserBase::STYPE__ tag) {
            //std::cout << "end tag:[" << tag << "]" << std::endl;
            if (tag.s == "textarea" || tag.s == "script") {
                --ignore;
            }
        }
        void reportIP(std::string ip, int pos, int pos_end) {
            if (curTag == "a") {
                std::cout << "Found ip in a/href: " << ip << " pos:[" << pos << "," << pos_end << ")" << std::endl;
                Operation op;
                op.start_pos = pos;
                op.end_pos = pos_end;
                op.op = Operation::REPLACE;
                    for (int i = 0; i < ip.size(); ++i) if (ip[i] == '.') ip[i] = ',';
                    op.newdata = ip;
//                op.newdata = "[1234::" + ip + "]";
                addOperation(op);
            }
        }
        bool chunked;
        int len;
        
};


#endif
