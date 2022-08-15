//
// Created by Jesse Vogel on 30/09/2021.
//

#include "Application.h"
#include "parser/Parser.h"
#include "core/macros.h"
#include "parser/Message.h"
#include <cstdlib>
#include <fstream>
#include <thread>
#include <chrono>

Application::Application(const std::vector<std::string> &arguments) {
    std::vector<std::string> files;
    std::string path_documentation;

    // Parse arguments
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
        const auto &arg = *it;
        if (arg == "--json") {
            m_options.json = true;
            continue;
        }
        if (arg == "--namespaces") {
            m_options.show_namespaces = true;
            continue;
        }
        if (arg == "--threads") {
            if (++it == arguments.end()) {
                CANARD_LOG("Number of threads missing");
                continue;
            }
            int max = (int) std::thread::hardware_concurrency();
            try {
                m_options.max_search_threads = (*it == "max") ? max : std::min(max, std::stoi(*it));
            } catch (const std::exception &e) {
                CANARD_LOG("Invalid number of threads");
            }
            continue;
        }
        if (arg == "--docs") {
            m_options.documentation = true;
            if (++it == arguments.end()) {
                CANARD_LOG("Path for documentation file missing");
                continue;
            }
            path_documentation = *it;
            continue;
        }
        files.push_back(arg);
    }

    // Parse files
    for (const auto &file: files) {
        if (!parse_file(file))
            CANARD_LOG("Failed to parse '" << file << "'");
    }

    // Write documentation if
    if (m_options.documentation)
        write_documentation(path_documentation);
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
        CANARD_LOG("Invalid path '" << path << "'");
        return false;
    }

    // Create input file stream
    std::ifstream input(n_path);
    if (!input.good()) {
        CANARD_LOG("Could not open '" << n_path << "'");
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

void Application::write_documentation(const std::string &file) {
    std::ofstream output(file);
    if (output.fail()) {
        CANARD_LOG("Failed to write documentation '" << file << "'");
        return;
    }

    output << "{";
    for (const auto &entry: m_documentation) {
        if (!entry.second.empty())
            output << "\"" << entry.first << "\":\"" << Message::json_escape(entry.second) << "\","; // TODO: trim entry.second
    }
    output.seekp(-1, std::ios_base::cur);
    output << "}" << std::endl;
}
