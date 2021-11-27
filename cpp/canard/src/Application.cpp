//
// Created by Jesse Vogel on 30/09/2021.
//

#include "Application.h"
#include "parser/Parser.h"
#include "core/macros.h"
#include "core/Formatter.h"
#include <cstdlib>
#include <fstream>
#include <thread>
#include <chrono>

Application::Application(const std::vector<std::string> &arguments) {
    // Read flags
    for (const auto &arg: arguments) {
        if (arg == "--json") m_options.json = true;
        if (arg == "--explicit") m_options.explict = true;
        if (arg == "--doc") m_options.documentation = true;
        if (arg == "--multithreading") m_options.max_search_threads = std::thread::hardware_concurrency();
    }

    // Parse files
    for (const auto &arg: arguments) {
        if (arg.rfind("--", 0) == 0) continue; // Skip flags

        if (!parse_file(arg))
            CANARD_LOG("Failed to parse `" << arg << "`");
    }
}

void Application::run() {
    // Create parser and keep parsing (until exit) via System.in
    Parser parser(std::cin, std::cout, m_session, m_options);
    parser.set_documentation(&m_documentation);
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
    Parser parser(input, std::cout, m_session, m_options);
    parser.set_location(directory, file);
    parser.set_documentation(&m_documentation);
    auto start_time = std::chrono::system_clock::now();
    bool success = parser.parse();
    auto end_time = std::chrono::system_clock::now();
    CANARD_LOG("Importing " << n_path << " took "
                            << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()
                            << " ms");
    return success;
}
