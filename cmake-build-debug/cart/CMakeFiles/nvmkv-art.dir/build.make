# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.14

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/wangke/CLionProjects/nvmkv

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/wangke/CLionProjects/nvmkv/cmake-build-debug

# Include any dependencies generated for this target.
include cart/CMakeFiles/nvmkv-art.dir/depend.make

# Include the progress variables for this target.
include cart/CMakeFiles/nvmkv-art.dir/progress.make

# Include the compile flags for this target's objects.
include cart/CMakeFiles/nvmkv-art.dir/flags.make

cart/CMakeFiles/nvmkv-art.dir/art.cpp.o: cart/CMakeFiles/nvmkv-art.dir/flags.make
cart/CMakeFiles/nvmkv-art.dir/art.cpp.o: ../cart/art.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object cart/CMakeFiles/nvmkv-art.dir/art.cpp.o"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/nvmkv-art.dir/art.cpp.o -c /Users/wangke/CLionProjects/nvmkv/cart/art.cpp

cart/CMakeFiles/nvmkv-art.dir/art.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/nvmkv-art.dir/art.cpp.i"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/wangke/CLionProjects/nvmkv/cart/art.cpp > CMakeFiles/nvmkv-art.dir/art.cpp.i

cart/CMakeFiles/nvmkv-art.dir/art.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/nvmkv-art.dir/art.cpp.s"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/wangke/CLionProjects/nvmkv/cart/art.cpp -o CMakeFiles/nvmkv-art.dir/art.cpp.s

cart/CMakeFiles/nvmkv-art.dir/art_node.cpp.o: cart/CMakeFiles/nvmkv-art.dir/flags.make
cart/CMakeFiles/nvmkv-art.dir/art_node.cpp.o: ../cart/art_node.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object cart/CMakeFiles/nvmkv-art.dir/art_node.cpp.o"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/nvmkv-art.dir/art_node.cpp.o -c /Users/wangke/CLionProjects/nvmkv/cart/art_node.cpp

cart/CMakeFiles/nvmkv-art.dir/art_node.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/nvmkv-art.dir/art_node.cpp.i"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/wangke/CLionProjects/nvmkv/cart/art_node.cpp > CMakeFiles/nvmkv-art.dir/art_node.cpp.i

cart/CMakeFiles/nvmkv-art.dir/art_node.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/nvmkv-art.dir/art_node.cpp.s"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/wangke/CLionProjects/nvmkv/cart/art_node.cpp -o CMakeFiles/nvmkv-art.dir/art_node.cpp.s

# Object files for target nvmkv-art
nvmkv__art_OBJECTS = \
"CMakeFiles/nvmkv-art.dir/art.cpp.o" \
"CMakeFiles/nvmkv-art.dir/art_node.cpp.o"

# External object files for target nvmkv-art
nvmkv__art_EXTERNAL_OBJECTS =

cart/libnvmkv-art.a: cart/CMakeFiles/nvmkv-art.dir/art.cpp.o
cart/libnvmkv-art.a: cart/CMakeFiles/nvmkv-art.dir/art_node.cpp.o
cart/libnvmkv-art.a: cart/CMakeFiles/nvmkv-art.dir/build.make
cart/libnvmkv-art.a: cart/CMakeFiles/nvmkv-art.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library libnvmkv-art.a"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && $(CMAKE_COMMAND) -P CMakeFiles/nvmkv-art.dir/cmake_clean_target.cmake
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/nvmkv-art.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
cart/CMakeFiles/nvmkv-art.dir/build: cart/libnvmkv-art.a

.PHONY : cart/CMakeFiles/nvmkv-art.dir/build

cart/CMakeFiles/nvmkv-art.dir/clean:
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && $(CMAKE_COMMAND) -P CMakeFiles/nvmkv-art.dir/cmake_clean.cmake
.PHONY : cart/CMakeFiles/nvmkv-art.dir/clean

cart/CMakeFiles/nvmkv-art.dir/depend:
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/wangke/CLionProjects/nvmkv /Users/wangke/CLionProjects/nvmkv/cart /Users/wangke/CLionProjects/nvmkv/cmake-build-debug /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart/CMakeFiles/nvmkv-art.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : cart/CMakeFiles/nvmkv-art.dir/depend

