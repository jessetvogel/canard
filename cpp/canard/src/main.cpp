#include "Application.h"
#include "core/macros.h"

int main(int argc, char *argv[]) {
    // Construct vector of arguments
    std::vector<std::string> arguments;
    arguments.reserve(argc - 1);
    for (int i = 1; i < argc; ++i)
        arguments.emplace_back(argv[i]);

    // Create and run Application
    Application application(arguments);
    application.run();
    return 0;
}
