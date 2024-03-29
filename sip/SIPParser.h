// Generated by Bisonc++ V4.05.00 on Mon, 15 Sep 2014 23:21:21 +0800

#ifndef SIPParser_h_included
#define SIPParser_h_included

// $insert baseclass
#include "SIPParserbase.h"
// $insert scanner.h
#include "SIPScanner.h"

#include "../parser.h"
#include "../state.h"

#undef SIPParser
class SIPParser: public SIPParserBase, public Parser
{
    // $insert scannerobject
    SIPScanner d_scanner;
        
    public:
        SIPParser(Communicator *c)
          : Parser(c),
            c_(c) {
            d_scanner.setSval(&d_val__);
        }
        int parse();
        
        virtual void run__() {
            //puts("SIP run!");
            d_scanner.switchStreams(*(c_->getIStream()));
            parse();
        }

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
        Communicator *c_;
        TokenType c_ip_;
        TokenType content_len_;
};


#endif
