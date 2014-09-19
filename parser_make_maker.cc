#include <cstdio>
#include <cctype>
#include <cstring>
int main(int argc, char **argv)
{
    freopen("parser_make.h", "w", stdout);
        
    puts("#pragma once");
    puts("#include \"parser.h\"\n");
    puts("ParserPtr Parser::make(std::string protocol, Communicator *c) {");
    for (int i = 1; i < argc; ++i) {
        printf("    if (protocol == \"%s\")\n", argv[i]);
        printf("        return ParserPtr(new ");
        for (int j = 0; j < strlen(argv[i]); ++j)
            putchar(toupper(argv[i][j]));
        printf("Parser(c));\n");
    }
    puts("    return ParserPtr();\n}\n");
}
