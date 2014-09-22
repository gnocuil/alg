#include <cstdio>
#include <cctype>
#include <cstring>
int main(int argc, char **argv)
{
    freopen("parser_make.h", "w", stdout);
        
    puts("#pragma once");
    puts("#include \"parser.h\"\n");
    
    for (int i = 1; i < argc; ++i) {
        printf("#include \"%s/", argv[i]);
        for (int j = 0; j < strlen(argv[i]); ++j)
            putchar(toupper(argv[i][j]));
        printf("Parser.h\"\n");
    }
    
    puts("ParserPtr Parser::make(std::string protocol, Communicator *c, Flow* flow_) {");
    puts("    ParserPtr ret = ParserPtr();");
    for (int i = 1; i < argc; ++i) {
        printf("    if (protocol == \"%s\")\n", argv[i]);
        printf("        ret = ParserPtr(new ");
        for (int j = 0; j < strlen(argv[i]); ++j)
            putchar(toupper(argv[i][j]));
        printf("Parser(c));\n");
    }
    puts("    if (ret) ret->flow = flow_;");
    puts("    return ret;\n}\n");
}
