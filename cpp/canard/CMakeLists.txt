cmake_minimum_required(VERSION 3.17)
project(canard)

# C++ version and optimization level
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Source files
add_executable(canard src/main.cpp src/core/Context.cpp src/core/Context.h src/core/Function.cpp src/core/Function.h src/core/Session.cpp src/core/Session.h src/core/Matcher.cpp src/core/Matcher.h src/core/Namespace.cpp src/core/Namespace.h src/core/Specialization.cpp src/core/Specialization.h src/parser/Lexer.cpp src/parser/Lexer.h src/parser/Parser.cpp src/parser/Parser.h src/parser/Scanner.cpp src/parser/Scanner.h src/parser/Message.cpp src/parser/Message.h src/searcher/Query.cpp src/searcher/Query.h src/searcher/Searcher.cpp src/searcher/Searcher.h src/core/macros.h src/Application.cpp src/Application.h src/core/Formatter.cpp src/core/Formatter.h src/searcher/ThreadManager.cpp src/searcher/ThreadManager.h)

# Enable multithreading
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(canard PRIVATE Threads::Threads)
