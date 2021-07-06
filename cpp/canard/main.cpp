#include <iostream>
#include "core/session.h"
#include "parser/parser.h"

int main() {
    // Create session
    Session session;

    // Create parser
    Parser parser(std::cin, std::cout, session);

    // Keep parsing (until exit) via System.in
    parser.parse();

    return 0;
}
