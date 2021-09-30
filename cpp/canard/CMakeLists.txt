cmake_minimum_required(VERSION 3.17)
project(canard)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(canard main.cpp core/context.cpp core/context.h core/function.cpp core/function.h core/session.cpp core/session.h core/matcher.cpp core/matcher.h core/namespace.cpp core/namespace.h core/specialization.cpp core/specialization.h parser/lexer.cpp parser/lexer.h parser/parser.cpp parser/parser.h parser/scanner.cpp parser/scanner.h parser/message.cpp parser/message.h searcher/query.cpp searcher/query.h searcher/searcher.cpp searcher/searcher.h core/macros.h)
