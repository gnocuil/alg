// Generated by Flexc++ V1.08.00 on Mon, 15 Sep 2014 23:21:21 +0800

#ifndef SIPScanner_H_INCLUDED_
#define SIPScanner_H_INCLUDED_

// $insert baseclass_h
#include "SIPScannerbase.h"

#include "SIPParserbase.h"

// $insert classHead
class SIPScanner: public SIPScannerBase
{
    public:
        explicit SIPScanner(std::istream &in = std::cin,
                                std::ostream &out = std::cout);

        SIPScanner(std::string const &infile, std::string const &outfile);
        
        // $insert lexFunctionDecl
        int lex();
        
        void setSval(SIPParserBase::STYPE__ *d_val_) { d_val = d_val_; }

    private:
        int lex__();
        int executeAction__(size_t ruleNr);

        void print();
        void preCode();     // re-implement this function for code that must 
                            // be exec'ed before the patternmatching starts

        void postCode(PostEnum__ type);    
                            // re-implement this function for code that must 
                            // be exec'ed after the rules's actions.
        SIPParserBase::STYPE__ *d_val;
        int cnt;
        StartCondition__ nxt;
};

// $insert scannerConstructors
inline SIPScanner::SIPScanner(std::istream &in, std::ostream &out)
:
    SIPScannerBase(in, out),
    cnt(0)
{}

inline SIPScanner::SIPScanner(std::string const &infile, std::string const &outfile)
:
    SIPScannerBase(infile, outfile),
    cnt(0)
{}

// $insert inlineLexFunction
inline int SIPScanner::lex()
{
    return lex__();
}

inline void SIPScanner::preCode() 
{
    // optionally replace by your own code
}

inline void SIPScanner::postCode(PostEnum__ type) 
{
    // optionally replace by your own code
    if (d_val) *d_val = TokenType(matched(), cnt, cnt + matched().size());
    cnt += matched().size();
}

inline void SIPScanner::print() 
{
    print__();
}


#endif // SIPScanner_H_INCLUDED_

