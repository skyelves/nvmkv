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
include allocator/CMakeFiles/nvmkv-allocator.dir/depend.make

# Include the progress variables for this target.
include allocator/CMakeFiles/nvmkv-allocator.dir/progress.make

# Include the compile flags for this target's objects.
include allocator/CMakeFiles/nvmkv-allocator.dir/flags.make

allocator/CMakeFiles/nvmkv-allocator.dir/allocator.cpp.o: allocator/CMakeFiles/nvmkv-allocator.dir/flags.make
allocator/CMakeFiles/nvmkv-allocator.dir/allocator.cpp.o: ../allocator/allocator.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object allocator/CMakeFiles/nvmkv-allocator.dir/allocator.cpp.o"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/allocator && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/nvmkv-allocator.dir/allocator.cpp.o -c /Users/wangke/CLionProjects/nvmkv/allocator/allocator.cpp

allocator/CMakeFiles/nvmkv-allocator.dir/allocator.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/nvmkv-allocator.dir/allocator.cpp.i"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/allocator && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/wangke/CLionProjects/nvmkv/allocator/allocator.cpp > CMakeFiles/nvmkv-allocator.dir/allocator.cpp.i

allocator/CMakeFiles/nvmkv-allocator.dir/allocator.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/nvmkv-allocator.dir/allocator.cpp.s"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/allocator && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/wangke/CLionProjects/nvmkv/allocator/allocator.cpp -o CMakeFiles/nvmkv-allocator.dir/allocator.cpp.s

# Object files for target nvmkv-allocator
nvmkv__allocator_OBJECTS = \
"CMakeFiles/nvmkv-allocator.dir/allocator.cpp.o"

# External object files for target nvmkv-allocator
nvmkv__allocator_EXTERNAL_OBJECTS =

allocator/libnvmkv-allocator.a: allocator/CMakeFiles/nvmkv-allocator.dir/allocator.cpp.o
allocator/libnvmkv-allocator.a: allocator/CMakeFiles/nvmkv-allocator.dir/build.make
allocator/libnvmkv-allocator.a: allocator/CMakeFiles/nvmkv-allocator.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libnvmkv-allocator.a"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/allocator && $(CMAKE_COMMAND) -P CMakeFiles/nvmkv-allocator.dir/cmake_clean_target.cmake
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/allocator && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/nvmkv-allocator.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
allocator/CMakeFiles/nvmkv-allocator.dir/build: allocator/libnvmkv-allocator.a

.PHONY : allocator/CMakeFiles/nvmkv-allocator.dir/build

allocator/CMakeFiles/nvmkv-allocator.dir/clean:
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/allocator && $(CMAKE_COMMAND) -P CMakeFiles/nvmkv-allocator.dir/cmake_clean.cmake
.PHONY : allocator/CMakeFiles/nvmkv-allocator.dir/clean

allocator/CMakeFiles/nvmkv-allocator.dir/depend:
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/wangke/CLionProjects/nvmkv /Users/wangke/CLionProjects/nvmkv/allocator /Users/wangke/CLionProjects/nvmkv/cmake-build-debug /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/allocator /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/allocator/CMakeFiles/nvmkv-allocator.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : allocator/CMakeFiles/nvmkv-allocator.dir/depend

