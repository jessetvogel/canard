//
// Created by Jesse Vogel on 30/09/2021.
//

#include "Application.h"
#include "parser/Parser.h"
#include "core/macros.h"
#include "core/Formatter.h"
#include "formatter/EnglishFormatter.h"
#include <cstdlib>
#include <fstream>

Application::Application(const std::vector<std::string> &arguments) {
    // Read flags
    for (auto &arg: arguments) {
        if (arg == "--json") m_flag_json = true;
        if (arg == "--explicit") m_flag_explicit = true;
        if (arg == "--doc") m_flag_documentation = true;
        if (arg == "--english") m_flag_english = true;
    }

    // Parse files
    for (auto &arg: arguments) {
        if (arg.rfind("--", 0) == 0) continue; // Skip flags

        if (!parse_file(arg))
            CANARD_LOG("Failed to parse `" << arg << "`");
    }

}

void Application::run() {
    // Create English formatter
    EnglishFormatter formatter;

    // Create parser and keep parsing (until exit) via System.in
    Parser parser(std::cin, std::cout, m_session);
    parser_set_flags(parser);
    if (m_flag_english) parser.set_formatter(formatter);
    parser.parse();
}

#ifndef PATH_MAX
#define PATH_MAX (1024)
#endif

std::string normalize_path(const std::string &path) {
    // Convert path to normalized path
    char normalized_path[PATH_MAX]; // PATH_MAX includes the \0 so +1 is not required
    char *result = realpath(path.c_str(), normalized_path);
    if (!result) return "";
    return normalized_path;
}

bool Application::parse_file(const std::string &path) {
    const std::string n_path = normalize_path(path);
    if (n_path.empty()) {
        CANARD_LOG("invalid path " << path);
        return false;
    }

    // Create input file stream
    std::ifstream input(n_path);
    if (!input.good()) {
        std::cout << "could not open '" << n_path << "'" << std::endl;
        return false;
    }

    // Obtain directory and filename
    size_t j = n_path.find_last_of('/');
    std::string directory = n_path.substr(0, j + 1);
    std::string file = n_path.substr(j + 1);

    // Create parser to parse the file
    Parser parser(input, std::cout, m_session);
    parser_set_flags(parser);
    parser.set_location(directory, file);
    return parser.parse();
}

void Application::parser_set_flags(Parser &parser) {
    parser.use_json(m_flag_json);
    parser.use_explicit(m_flag_explicit);
    if (m_flag_documentation)
        parser.set_documentation(&m_documentation);
}