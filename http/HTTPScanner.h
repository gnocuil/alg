// Generated by Flexc++ V1.08.00 on Thu, 04 Sep 2014 23:59:59 +0800

#ifndef HTTPScanner_H_INCLUDED_
#define HTTPScanner_H_INCLUDED_

// $insert baseclass_h
#include "HTTPScannerbase.h"
#include "HTTPParserbase.h"
#include <sstream>
#include "../communicator.h"

// $insert classHead
class HTTPScanner: public HTTPScannerBase
{
    public:
        explicit HTTPScanner(std::istream &in = std::cin,
                                std::ostream &out = std::cout);

        HTTPScanner(std::string const &infile, std::string const &outfile);
        
        // $insert lexFunctionDecl
        int lex();
        
        void setSval(HTTPParserBase::STYPE__ *d_val_) { d_val = d_val_; }
        
        int getCount() { return cnt; }

    private:
        int lex__();
        int executeAction__(size_t ruleNr);

        void print();
        void preCode();     // re-implement this function for code that must 
                            // be exec'ed before the patternmatching starts

        void postCode(PostEnum__ type);    
                            // re-implement this function for code that must 
                            // be exec'ed after the rules's actions.
        HTTPParserBase::STYPE__ *d_val;
        bool inURL, urlFirst;
        int cnt;
        bool ignore;
};

// $insert scannerConstructors
inline HTTPScanner::HTTPScanner(std::istream &in, std::ostream &out)
:
    HTTPScannerBase(in, out),
    d_val(NULL),
    cnt(0),
    ignore(false) {
    
}

inline HTTPScanner::HTTPScanner(std::string const &infile, std::string const &outfile)
:
    HTTPScannerBase(infile, outfile),
    d_val(NULL),
    cnt(0),
    ignore(false)
{}

// $insert inlineLexFunction
inline int HTTPScanner::lex()
{
    return lex__();
}

inline void HTTPScanner::preCode() 
{
    // optionally replace by your own code
}

inline void HTTPScanner::postCode(PostEnum__ type) 
{
    // optionally replace by your own code
    if (d_val) *d_val = TokenType(matched(), cnt, cnt + matched().size());
    cnt += matched().size();
    //std::cout << "{" << matched() << "}" << std::endl;
    //if (matched().size() == 0) exit(1);
}

inline void HTTPScanner::print() 
{
    print__();
}


#endif // HTTPScanner_H_INCLUDED_

