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
include CMakeFiles/nvmkv.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/nvmkv.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/nvmkv.dir/flags.make

CMakeFiles/nvmkv.dir/main.cpp.o: CMakeFiles/nvmkv.dir/flags.make
CMakeFiles/nvmkv.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/nvmkv.dir/main.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/nvmkv.dir/main.cpp.o -c /Users/wangke/CLionProjects/nvmkv/main.cpp

CMakeFiles/nvmkv.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/nvmkv.dir/main.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/wangke/CLionProjects/nvmkv/main.cpp > CMakeFiles/nvmkv.dir/main.cpp.i

CMakeFiles/nvmkv.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/nvmkv.dir/main.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/wangke/CLionProjects/nvmkv/main.cpp -o CMakeFiles/nvmkv.dir/main.cpp.s

# Object files for target nvmkv
nvmkv_OBJECTS = \
"CMakeFiles/nvmkv.dir/main.cpp.o"

# External object files for target nvmkv
nvmkv_EXTERNAL_OBJECTS =

nvmkv: CMakeFiles/nvmkv.dir/main.cpp.o
nvmkv: CMakeFiles/nvmkv.dir/build.make
nvmkv: blink/libnvmkv-blink.a
nvmkv: art/libnvmkv-art.a
nvmkv: mass/libnvmkv-mass.a
nvmkv: rng/libnvmkv-rng.a
nvmkv: allocator/libnvmkv-allocator.a
nvmkv: node/libnvmkv-node.a
nvmkv: CMakeFiles/nvmkv.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable nvmkv"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/nvmkv.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/nvmkv.dir/build: nvmkv

.PHONY : CMakeFiles/nvmkv.dir/build

CMakeFiles/nvmkv.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/nvmkv.dir/cmake_clean.cmake
.PHONY : CMakeFiles/nvmkv.dir/clean

CMakeFiles/nvmkv.dir/depend:
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/wangke/CLionProjects/nvmkv /Users/wangke/CLionProjects/nvmkv /Users/wangke/CLionProjects/nvmkv/cmake-build-debug /Users/wangke/CLionProjects/nvmkv/cmake-build-debug /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles/nvmkv.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/nvmkv.dir/depend

