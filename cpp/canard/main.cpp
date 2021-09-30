#include <iostream>
#include <fstream>
#include <cstring>
#include "core/session.h"
#include "parser/parser.h"

bool parse_file(Session&, ParserFormat format, const char*);

int main(int argc, char *argv[]) {
    // Create session
    Session session;

    // Read options (starting with --)
    int format = PLAIN;
    for (int i = 1; i < argc; ++i) { // note: the first argument is the program itself!
        if (strcmp(argv[i], "--json") == 0)
            format |= JSON;
        if (strcmp(argv[i], "--explicit") == 0)
            format |= EXPLICIT;
    }

    // Parse files (not starting with --)
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--", 2) == 0)
            continue;

        if(!parse_file(session, (ParserFormat) format, argv[i]))
            return 0;
    }

    // Create parser
    Parser parser(std::cin, std::cout, session);
    parser.set_format((ParserFormat) format);

    // Keep parsing (until exit) via System.in
    parser.parse();

    return 0;
}

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

bool parse_file(Session& session, ParserFormat format, const char* path) {
    // Convert path to normalized path
    char normalized_path[PATH_MAX]; // PATH_MAX includes the \0 so +1 is not required
    char *result = realpath(path, normalized_path);
    if (!result) {
        if (errno == ENOENT) {
            std::cout << "could not find '" << path << "'" << std::endl;
            return false;
        }
        else {
            std::cout << "could not open '" << path << "'" << std::endl;
            return false;
        }
    }
    std::string s_path = normalized_path;

    // Create input file stream
    std::ifstream ifstream(s_path);
    if (!ifstream.good()) {
        std::cout << "could not open '" << s_path << "'" << std::endl;
        return false;
    }

    // Obtain directory and filename
    size_t j = s_path.find_last_of('/');
    std::string directory = s_path.substr(0, j + 1);
    std::string file = s_path.substr(j + 1);

    // Create sub_parser to parse the file
    Parser sub_parser(ifstream, std::cout, session);
    sub_parser.set_location(directory, file);
    sub_parser.set_format(format);
    return sub_parser.parse();
}
