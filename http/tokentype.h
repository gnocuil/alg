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
    friend std::string operator+ (const TokenType& a, std::string b) {
        return a.s + b;
    }
    friend std::string operator+ (std::string a, const TokenType& b) {
        return a + b.s;
    }
//private:
    std::string s;
    int pos;
    int pos_end;
};

#endif //TokenType_h_included
