#include <iostream>
#include "core/session.h"
#include "parser/parser.h"

int main(int argc, char *argv[]) {
    // Create session
    Session session;

    // Read options
    int format = PLAIN;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "--json") == 0)
            format |= JSON;
        if (strcmp(argv[i], "--explicit") == 0)
            format |= EXPLICIT;
    }

    // Create parser
    Parser parser(std::cin, std::cout, session);
    parser.set_format((ParserFormat) format);

    // Keep parsing (until exit) via System.in
    parser.parse();

    return 0;
}
