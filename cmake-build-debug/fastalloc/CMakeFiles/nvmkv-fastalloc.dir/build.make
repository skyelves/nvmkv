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
include fastalloc/CMakeFiles/nvmkv-fastalloc.dir/depend.make

# Include the progress variables for this target.
include fastalloc/CMakeFiles/nvmkv-fastalloc.dir/progress.make

# Include the compile flags for this target's objects.
include fastalloc/CMakeFiles/nvmkv-fastalloc.dir/flags.make

fastalloc/CMakeFiles/nvmkv-fastalloc.dir/fastalloc.cpp.o: fastalloc/CMakeFiles/nvmkv-fastalloc.dir/flags.make
fastalloc/CMakeFiles/nvmkv-fastalloc.dir/fastalloc.cpp.o: ../fastalloc/fastalloc.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object fastalloc/CMakeFiles/nvmkv-fastalloc.dir/fastalloc.cpp.o"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/fastalloc && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/nvmkv-fastalloc.dir/fastalloc.cpp.o -c /Users/wangke/CLionProjects/nvmkv/fastalloc/fastalloc.cpp

fastalloc/CMakeFiles/nvmkv-fastalloc.dir/fastalloc.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/nvmkv-fastalloc.dir/fastalloc.cpp.i"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/fastalloc && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/wangke/CLionProjects/nvmkv/fastalloc/fastalloc.cpp > CMakeFiles/nvmkv-fastalloc.dir/fastalloc.cpp.i

fastalloc/CMakeFiles/nvmkv-fastalloc.dir/fastalloc.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/nvmkv-fastalloc.dir/fastalloc.cpp.s"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/fastalloc && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/wangke/CLionProjects/nvmkv/fastalloc/fastalloc.cpp -o CMakeFiles/nvmkv-fastalloc.dir/fastalloc.cpp.s

# Object files for target nvmkv-fastalloc
nvmkv__fastalloc_OBJECTS = \
"CMakeFiles/nvmkv-fastalloc.dir/fastalloc.cpp.o"

# External object files for target nvmkv-fastalloc
nvmkv__fastalloc_EXTERNAL_OBJECTS =

fastalloc/libnvmkv-fastalloc.a: fastalloc/CMakeFiles/nvmkv-fastalloc.dir/fastalloc.cpp.o
fastalloc/libnvmkv-fastalloc.a: fastalloc/CMakeFiles/nvmkv-fastalloc.dir/build.make
fastalloc/libnvmkv-fastalloc.a: fastalloc/CMakeFiles/nvmkv-fastalloc.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libnvmkv-fastalloc.a"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/fastalloc && $(CMAKE_COMMAND) -P CMakeFiles/nvmkv-fastalloc.dir/cmake_clean_target.cmake
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/fastalloc && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/nvmkv-fastalloc.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
fastalloc/CMakeFiles/nvmkv-fastalloc.dir/build: fastalloc/libnvmkv-fastalloc.a

.PHONY : fastalloc/CMakeFiles/nvmkv-fastalloc.dir/build

fastalloc/CMakeFiles/nvmkv-fastalloc.dir/clean:
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/fastalloc && $(CMAKE_COMMAND) -P CMakeFiles/nvmkv-fastalloc.dir/cmake_clean.cmake
.PHONY : fastalloc/CMakeFiles/nvmkv-fastalloc.dir/clean

fastalloc/CMakeFiles/nvmkv-fastalloc.dir/depend:
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/wangke/CLionProjects/nvmkv /Users/wangke/CLionProjects/nvmkv/fastalloc /Users/wangke/CLionProjects/nvmkv/cmake-build-debug /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/fastalloc /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/fastalloc/CMakeFiles/nvmkv-fastalloc.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : fastalloc/CMakeFiles/nvmkv-fastalloc.dir/depend

