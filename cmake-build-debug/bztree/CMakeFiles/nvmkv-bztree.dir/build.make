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
include bztree/CMakeFiles/nvmkv-bztree.dir/depend.make

# Include the progress variables for this target.
include bztree/CMakeFiles/nvmkv-bztree.dir/progress.make

# Include the compile flags for this target's objects.
include bztree/CMakeFiles/nvmkv-bztree.dir/flags.make

bztree/CMakeFiles/nvmkv-bztree.dir/bztree.cpp.o: bztree/CMakeFiles/nvmkv-bztree.dir/flags.make
bztree/CMakeFiles/nvmkv-bztree.dir/bztree.cpp.o: ../bztree/bztree.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object bztree/CMakeFiles/nvmkv-bztree.dir/bztree.cpp.o"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bztree && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/nvmkv-bztree.dir/bztree.cpp.o -c /Users/wangke/CLionProjects/nvmkv/bztree/bztree.cpp

bztree/CMakeFiles/nvmkv-bztree.dir/bztree.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/nvmkv-bztree.dir/bztree.cpp.i"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bztree && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/wangke/CLionProjects/nvmkv/bztree/bztree.cpp > CMakeFiles/nvmkv-bztree.dir/bztree.cpp.i

bztree/CMakeFiles/nvmkv-bztree.dir/bztree.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/nvmkv-bztree.dir/bztree.cpp.s"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bztree && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/wangke/CLionProjects/nvmkv/bztree/bztree.cpp -o CMakeFiles/nvmkv-bztree.dir/bztree.cpp.s

# Object files for target nvmkv-bztree
nvmkv__bztree_OBJECTS = \
"CMakeFiles/nvmkv-bztree.dir/bztree.cpp.o"

# External object files for target nvmkv-bztree
nvmkv__bztree_EXTERNAL_OBJECTS =

bztree/libnvmkv-bztree.a: bztree/CMakeFiles/nvmkv-bztree.dir/bztree.cpp.o
bztree/libnvmkv-bztree.a: bztree/CMakeFiles/nvmkv-bztree.dir/build.make
bztree/libnvmkv-bztree.a: bztree/CMakeFiles/nvmkv-bztree.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libnvmkv-bztree.a"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bztree && $(CMAKE_COMMAND) -P CMakeFiles/nvmkv-bztree.dir/cmake_clean_target.cmake
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bztree && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/nvmkv-bztree.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
bztree/CMakeFiles/nvmkv-bztree.dir/build: bztree/libnvmkv-bztree.a

.PHONY : bztree/CMakeFiles/nvmkv-bztree.dir/build

bztree/CMakeFiles/nvmkv-bztree.dir/clean:
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bztree && $(CMAKE_COMMAND) -P CMakeFiles/nvmkv-bztree.dir/cmake_clean.cmake
.PHONY : bztree/CMakeFiles/nvmkv-bztree.dir/clean

bztree/CMakeFiles/nvmkv-bztree.dir/depend:
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/wangke/CLionProjects/nvmkv /Users/wangke/CLionProjects/nvmkv/bztree /Users/wangke/CLionProjects/nvmkv/cmake-build-debug /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bztree /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bztree/CMakeFiles/nvmkv-bztree.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : bztree/CMakeFiles/nvmkv-bztree.dir/depend

