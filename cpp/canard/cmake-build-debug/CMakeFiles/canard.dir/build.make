# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.20

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake

# The command to remove a file.
RM = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/jessevogel/Projects/canard/cpp/canard

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/canard.dir/depend.make
# Include the progress variables for this target.
include CMakeFiles/canard.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/canard.dir/flags.make

CMakeFiles/canard.dir/main.cpp.o: CMakeFiles/canard.dir/flags.make
CMakeFiles/canard.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/canard.dir/main.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/canard.dir/main.cpp.o -c /Users/jessevogel/Projects/canard/cpp/canard/main.cpp

CMakeFiles/canard.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/canard.dir/main.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jessevogel/Projects/canard/cpp/canard/main.cpp > CMakeFiles/canard.dir/main.cpp.i

CMakeFiles/canard.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/canard.dir/main.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jessevogel/Projects/canard/cpp/canard/main.cpp -o CMakeFiles/canard.dir/main.cpp.s

CMakeFiles/canard.dir/core/context.cpp.o: CMakeFiles/canard.dir/flags.make
CMakeFiles/canard.dir/core/context.cpp.o: ../core/context.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/canard.dir/core/context.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/canard.dir/core/context.cpp.o -c /Users/jessevogel/Projects/canard/cpp/canard/core/context.cpp

CMakeFiles/canard.dir/core/context.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/canard.dir/core/context.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jessevogel/Projects/canard/cpp/canard/core/context.cpp > CMakeFiles/canard.dir/core/context.cpp.i

CMakeFiles/canard.dir/core/context.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/canard.dir/core/context.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jessevogel/Projects/canard/cpp/canard/core/context.cpp -o CMakeFiles/canard.dir/core/context.cpp.s

CMakeFiles/canard.dir/core/function.cpp.o: CMakeFiles/canard.dir/flags.make
CMakeFiles/canard.dir/core/function.cpp.o: ../core/function.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/canard.dir/core/function.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/canard.dir/core/function.cpp.o -c /Users/jessevogel/Projects/canard/cpp/canard/core/function.cpp

CMakeFiles/canard.dir/core/function.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/canard.dir/core/function.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jessevogel/Projects/canard/cpp/canard/core/function.cpp > CMakeFiles/canard.dir/core/function.cpp.i

CMakeFiles/canard.dir/core/function.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/canard.dir/core/function.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jessevogel/Projects/canard/cpp/canard/core/function.cpp -o CMakeFiles/canard.dir/core/function.cpp.s

CMakeFiles/canard.dir/core/session.cpp.o: CMakeFiles/canard.dir/flags.make
CMakeFiles/canard.dir/core/session.cpp.o: ../core/session.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/canard.dir/core/session.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/canard.dir/core/session.cpp.o -c /Users/jessevogel/Projects/canard/cpp/canard/core/session.cpp

CMakeFiles/canard.dir/core/session.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/canard.dir/core/session.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jessevogel/Projects/canard/cpp/canard/core/session.cpp > CMakeFiles/canard.dir/core/session.cpp.i

CMakeFiles/canard.dir/core/session.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/canard.dir/core/session.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jessevogel/Projects/canard/cpp/canard/core/session.cpp -o CMakeFiles/canard.dir/core/session.cpp.s

CMakeFiles/canard.dir/core/matcher.cpp.o: CMakeFiles/canard.dir/flags.make
CMakeFiles/canard.dir/core/matcher.cpp.o: ../core/matcher.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/canard.dir/core/matcher.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/canard.dir/core/matcher.cpp.o -c /Users/jessevogel/Projects/canard/cpp/canard/core/matcher.cpp

CMakeFiles/canard.dir/core/matcher.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/canard.dir/core/matcher.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jessevogel/Projects/canard/cpp/canard/core/matcher.cpp > CMakeFiles/canard.dir/core/matcher.cpp.i

CMakeFiles/canard.dir/core/matcher.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/canard.dir/core/matcher.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jessevogel/Projects/canard/cpp/canard/core/matcher.cpp -o CMakeFiles/canard.dir/core/matcher.cpp.s

CMakeFiles/canard.dir/core/namespace.cpp.o: CMakeFiles/canard.dir/flags.make
CMakeFiles/canard.dir/core/namespace.cpp.o: ../core/namespace.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/canard.dir/core/namespace.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/canard.dir/core/namespace.cpp.o -c /Users/jessevogel/Projects/canard/cpp/canard/core/namespace.cpp

CMakeFiles/canard.dir/core/namespace.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/canard.dir/core/namespace.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jessevogel/Projects/canard/cpp/canard/core/namespace.cpp > CMakeFiles/canard.dir/core/namespace.cpp.i

CMakeFiles/canard.dir/core/namespace.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/canard.dir/core/namespace.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jessevogel/Projects/canard/cpp/canard/core/namespace.cpp -o CMakeFiles/canard.dir/core/namespace.cpp.s

CMakeFiles/canard.dir/core/specialization.cpp.o: CMakeFiles/canard.dir/flags.make
CMakeFiles/canard.dir/core/specialization.cpp.o: ../core/specialization.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object CMakeFiles/canard.dir/core/specialization.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/canard.dir/core/specialization.cpp.o -c /Users/jessevogel/Projects/canard/cpp/canard/core/specialization.cpp

CMakeFiles/canard.dir/core/specialization.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/canard.dir/core/specialization.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jessevogel/Projects/canard/cpp/canard/core/specialization.cpp > CMakeFiles/canard.dir/core/specialization.cpp.i

CMakeFiles/canard.dir/core/specialization.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/canard.dir/core/specialization.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jessevogel/Projects/canard/cpp/canard/core/specialization.cpp -o CMakeFiles/canard.dir/core/specialization.cpp.s

CMakeFiles/canard.dir/parser/lexer.cpp.o: CMakeFiles/canard.dir/flags.make
CMakeFiles/canard.dir/parser/lexer.cpp.o: ../parser/lexer.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building CXX object CMakeFiles/canard.dir/parser/lexer.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/canard.dir/parser/lexer.cpp.o -c /Users/jessevogel/Projects/canard/cpp/canard/parser/lexer.cpp

CMakeFiles/canard.dir/parser/lexer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/canard.dir/parser/lexer.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jessevogel/Projects/canard/cpp/canard/parser/lexer.cpp > CMakeFiles/canard.dir/parser/lexer.cpp.i

CMakeFiles/canard.dir/parser/lexer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/canard.dir/parser/lexer.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jessevogel/Projects/canard/cpp/canard/parser/lexer.cpp -o CMakeFiles/canard.dir/parser/lexer.cpp.s

CMakeFiles/canard.dir/parser/parser.cpp.o: CMakeFiles/canard.dir/flags.make
CMakeFiles/canard.dir/parser/parser.cpp.o: ../parser/parser.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building CXX object CMakeFiles/canard.dir/parser/parser.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/canard.dir/parser/parser.cpp.o -c /Users/jessevogel/Projects/canard/cpp/canard/parser/parser.cpp

CMakeFiles/canard.dir/parser/parser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/canard.dir/parser/parser.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jessevogel/Projects/canard/cpp/canard/parser/parser.cpp > CMakeFiles/canard.dir/parser/parser.cpp.i

CMakeFiles/canard.dir/parser/parser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/canard.dir/parser/parser.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jessevogel/Projects/canard/cpp/canard/parser/parser.cpp -o CMakeFiles/canard.dir/parser/parser.cpp.s

CMakeFiles/canard.dir/parser/scanner.cpp.o: CMakeFiles/canard.dir/flags.make
CMakeFiles/canard.dir/parser/scanner.cpp.o: ../parser/scanner.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building CXX object CMakeFiles/canard.dir/parser/scanner.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/canard.dir/parser/scanner.cpp.o -c /Users/jessevogel/Projects/canard/cpp/canard/parser/scanner.cpp

CMakeFiles/canard.dir/parser/scanner.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/canard.dir/parser/scanner.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jessevogel/Projects/canard/cpp/canard/parser/scanner.cpp > CMakeFiles/canard.dir/parser/scanner.cpp.i

CMakeFiles/canard.dir/parser/scanner.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/canard.dir/parser/scanner.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jessevogel/Projects/canard/cpp/canard/parser/scanner.cpp -o CMakeFiles/canard.dir/parser/scanner.cpp.s

CMakeFiles/canard.dir/parser/message.cpp.o: CMakeFiles/canard.dir/flags.make
CMakeFiles/canard.dir/parser/message.cpp.o: ../parser/message.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building CXX object CMakeFiles/canard.dir/parser/message.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/canard.dir/parser/message.cpp.o -c /Users/jessevogel/Projects/canard/cpp/canard/parser/message.cpp

CMakeFiles/canard.dir/parser/message.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/canard.dir/parser/message.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jessevogel/Projects/canard/cpp/canard/parser/message.cpp > CMakeFiles/canard.dir/parser/message.cpp.i

CMakeFiles/canard.dir/parser/message.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/canard.dir/parser/message.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jessevogel/Projects/canard/cpp/canard/parser/message.cpp -o CMakeFiles/canard.dir/parser/message.cpp.s

CMakeFiles/canard.dir/searcher/query.cpp.o: CMakeFiles/canard.dir/flags.make
CMakeFiles/canard.dir/searcher/query.cpp.o: ../searcher/query.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building CXX object CMakeFiles/canard.dir/searcher/query.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/canard.dir/searcher/query.cpp.o -c /Users/jessevogel/Projects/canard/cpp/canard/searcher/query.cpp

CMakeFiles/canard.dir/searcher/query.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/canard.dir/searcher/query.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jessevogel/Projects/canard/cpp/canard/searcher/query.cpp > CMakeFiles/canard.dir/searcher/query.cpp.i

CMakeFiles/canard.dir/searcher/query.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/canard.dir/searcher/query.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jessevogel/Projects/canard/cpp/canard/searcher/query.cpp -o CMakeFiles/canard.dir/searcher/query.cpp.s

CMakeFiles/canard.dir/searcher/searcher.cpp.o: CMakeFiles/canard.dir/flags.make
CMakeFiles/canard.dir/searcher/searcher.cpp.o: ../searcher/searcher.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Building CXX object CMakeFiles/canard.dir/searcher/searcher.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/canard.dir/searcher/searcher.cpp.o -c /Users/jessevogel/Projects/canard/cpp/canard/searcher/searcher.cpp

CMakeFiles/canard.dir/searcher/searcher.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/canard.dir/searcher/searcher.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jessevogel/Projects/canard/cpp/canard/searcher/searcher.cpp > CMakeFiles/canard.dir/searcher/searcher.cpp.i

CMakeFiles/canard.dir/searcher/searcher.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/canard.dir/searcher/searcher.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jessevogel/Projects/canard/cpp/canard/searcher/searcher.cpp -o CMakeFiles/canard.dir/searcher/searcher.cpp.s

# Object files for target canard
canard_OBJECTS = \
"CMakeFiles/canard.dir/main.cpp.o" \
"CMakeFiles/canard.dir/core/context.cpp.o" \
"CMakeFiles/canard.dir/core/function.cpp.o" \
"CMakeFiles/canard.dir/core/session.cpp.o" \
"CMakeFiles/canard.dir/core/matcher.cpp.o" \
"CMakeFiles/canard.dir/core/namespace.cpp.o" \
"CMakeFiles/canard.dir/core/specialization.cpp.o" \
"CMakeFiles/canard.dir/parser/lexer.cpp.o" \
"CMakeFiles/canard.dir/parser/parser.cpp.o" \
"CMakeFiles/canard.dir/parser/scanner.cpp.o" \
"CMakeFiles/canard.dir/parser/message.cpp.o" \
"CMakeFiles/canard.dir/searcher/query.cpp.o" \
"CMakeFiles/canard.dir/searcher/searcher.cpp.o"

# External object files for target canard
canard_EXTERNAL_OBJECTS =

canard: CMakeFiles/canard.dir/main.cpp.o
canard: CMakeFiles/canard.dir/core/context.cpp.o
canard: CMakeFiles/canard.dir/core/function.cpp.o
canard: CMakeFiles/canard.dir/core/session.cpp.o
canard: CMakeFiles/canard.dir/core/matcher.cpp.o
canard: CMakeFiles/canard.dir/core/namespace.cpp.o
canard: CMakeFiles/canard.dir/core/specialization.cpp.o
canard: CMakeFiles/canard.dir/parser/lexer.cpp.o
canard: CMakeFiles/canard.dir/parser/parser.cpp.o
canard: CMakeFiles/canard.dir/parser/scanner.cpp.o
canard: CMakeFiles/canard.dir/parser/message.cpp.o
canard: CMakeFiles/canard.dir/searcher/query.cpp.o
canard: CMakeFiles/canard.dir/searcher/searcher.cpp.o
canard: CMakeFiles/canard.dir/build.make
canard: CMakeFiles/canard.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_14) "Linking CXX executable canard"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/canard.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/canard.dir/build: canard
.PHONY : CMakeFiles/canard.dir/build

CMakeFiles/canard.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/canard.dir/cmake_clean.cmake
.PHONY : CMakeFiles/canard.dir/clean

CMakeFiles/canard.dir/depend:
	cd /Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/jessevogel/Projects/canard/cpp/canard /Users/jessevogel/Projects/canard/cpp/canard /Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug /Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug /Users/jessevogel/Projects/canard/cpp/canard/cmake-build-debug/CMakeFiles/canard.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/canard.dir/depend

