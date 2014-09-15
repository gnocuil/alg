#ifndef TokenType_h_included
#define TokenType_h_included
#include <string>

class TokenType {
public:
    TokenType(std::string s_ = "", int pos_ = 0, int pos_end_ = 0)
        : s(s_),
          pos(pos_),
          pos_end(pos_end_) {}
          
    friend std::ostream& operator<< (std::ostream& os, const TokenType& t) {
        return os << t.s;
    }
    friend TokenType operator+ (const TokenType& a, std::string b) {
        return TokenType(a.s + b, a.pos, a.pos_end);
    }
    friend TokenType operator+ (std::string a, const TokenType& b) {
        return TokenType(a + b.s, b.pos, b.pos_end);
    }
    friend TokenType operator+ (const TokenType& a, const TokenType& b) {
        return TokenType(a.s + b.s, a.pos, b.pos_end);
    }
    int Int() {
        return atoi(s.c_str());
    }
//private:
    std::string s;
    int pos;
    int pos_end;
};

#endif //TokenType_h_included
