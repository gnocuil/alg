// Generated by Flexc++ V1.08.00 on Fri, 12 Sep 2014 02:03:12 +0800

#ifndef FTPScanner_H_INCLUDED_
#define FTPScanner_H_INCLUDED_

// $insert baseclass_h
#include "FTPScannerbase.h"

#include "FTPParserbase.h"

// $insert classHead
class FTPScanner: public FTPScannerBase
{
    public:
        FTPScanner(std::istream &in = std::cin,
                                std::ostream &out = std::cout);

        FTPScanner(std::string const &infile, std::string const &outfile);
        
        // $insert lexFunctionDecl
        int lex();
        
        void setSval(FTPParserBase::STYPE__ *d_val_) { d_val = d_val_; }

    private:
        int lex__();
        int executeAction__(size_t ruleNr);

        void print();
        void preCode();     // re-implement this function for code that must 
                            // be exec'ed before the patternmatching starts

        void postCode(PostEnum__ type);    
                            // re-implement this function for code that must 
                            // be exec'ed after the rules's actions.
        FTPParserBase::STYPE__ *d_val;
};

// $insert scannerConstructors
inline FTPScanner::FTPScanner(std::istream &in, std::ostream &out)
:
    FTPScannerBase(in, out)
{}

inline FTPScanner::FTPScanner(std::string const &infile, std::string const &outfile)
:
    FTPScannerBase(infile, outfile)
{}

// $insert inlineLexFunction
inline int FTPScanner::lex()
{
    return lex__();
}

inline void FTPScanner::preCode() 
{
    // optionally replace by your own code
}

inline void FTPScanner::postCode(PostEnum__ type) 
{
    // optionally replace by your own code
    static int cnt = 0;
    if (d_val) *d_val = TokenType(matched(), cnt, cnt + matched().size());
    cnt += matched().size();
}

inline void FTPScanner::print() 
{
    print__();
}


#endif // FTPScanner_H_INCLUDED_

