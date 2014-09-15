// Generated by Bisonc++ V4.05.00 on Mon, 15 Sep 2014 11:50:50 +0800

#ifndef HTTPParserBase_h_included
#define HTTPParserBase_h_included

#include <exception>
#include <vector>
#include <iostream>

// $insert preincludes
#include "tokentype.h"

namespace // anonymous
{
    struct PI__;
}



class HTTPParserBase
{
    public:
// $insert tokens

    // Symbolic tokens:
    enum Tokens__
    {
        STATUS_IDENT = 257,
        INTEGER,
        HEADER_IDENT,
        HEADER_VALUE,
        TAG_IDENT,
        TAG_IGNORE,
        ATT_VALUE,
        TAG_IDENT_HREF,
        URL_NAME,
        URL_IDENT,
    };

// $insert STYPE
typedef TokenType STYPE__;


    private:
        int d_stackIdx__;
        std::vector<size_t>   d_stateStack__;
        std::vector<STYPE__>  d_valueStack__;

    protected:
        enum Return__
        {
            PARSE_ACCEPT__ = 0,   // values used as parse()'s return values
            PARSE_ABORT__  = 1
        };
        enum ErrorRecovery__
        {
            DEFAULT_RECOVERY_MODE__,
            UNEXPECTED_TOKEN__,
        };
        bool        d_debug__;
        size_t      d_nErrors__;
        size_t      d_requiredTokens__;
        size_t      d_acceptedTokens__;
        int         d_token__;
        int         d_nextToken__;
        size_t      d_state__;
        STYPE__    *d_vsp__;
        STYPE__     d_val__;
        STYPE__     d_nextVal__;

        HTTPParserBase();

        void ABORT() const;
        void ACCEPT() const;
        void ERROR() const;
        void clearin();
        bool debug() const;
        void pop__(size_t count = 1);
        void push__(size_t nextState);
        void popToken__();
        void pushToken__(int token);
        void reduce__(PI__ const &productionInfo);
        void errorVerbose__();
        size_t top__() const;

    public:
        void setDebug(bool mode);
}; 

inline bool HTTPParserBase::debug() const
{
    return d_debug__;
}

inline void HTTPParserBase::setDebug(bool mode)
{
    d_debug__ = mode;
}

inline void HTTPParserBase::ABORT() const
{
    throw PARSE_ABORT__;
}

inline void HTTPParserBase::ACCEPT() const
{
    throw PARSE_ACCEPT__;
}

inline void HTTPParserBase::ERROR() const
{
    throw UNEXPECTED_TOKEN__;
}


// As a convenience, when including ParserBase.h its symbols are available as
// symbols in the class Parser, too.
#define HTTPParser HTTPParserBase


#endif

