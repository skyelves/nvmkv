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
include bobhash/CMakeFiles/nvmkv-bobhash32.dir/depend.make

# Include the progress variables for this target.
include bobhash/CMakeFiles/nvmkv-bobhash32.dir/progress.make

# Include the compile flags for this target's objects.
include bobhash/CMakeFiles/nvmkv-bobhash32.dir/flags.make

bobhash/CMakeFiles/nvmkv-bobhash32.dir/bobhash32.cpp.o: bobhash/CMakeFiles/nvmkv-bobhash32.dir/flags.make
bobhash/CMakeFiles/nvmkv-bobhash32.dir/bobhash32.cpp.o: ../bobhash/bobhash32.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object bobhash/CMakeFiles/nvmkv-bobhash32.dir/bobhash32.cpp.o"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bobhash && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/nvmkv-bobhash32.dir/bobhash32.cpp.o -c /Users/wangke/CLionProjects/nvmkv/bobhash/bobhash32.cpp

bobhash/CMakeFiles/nvmkv-bobhash32.dir/bobhash32.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/nvmkv-bobhash32.dir/bobhash32.cpp.i"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bobhash && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/wangke/CLionProjects/nvmkv/bobhash/bobhash32.cpp > CMakeFiles/nvmkv-bobhash32.dir/bobhash32.cpp.i

bobhash/CMakeFiles/nvmkv-bobhash32.dir/bobhash32.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/nvmkv-bobhash32.dir/bobhash32.cpp.s"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bobhash && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/wangke/CLionProjects/nvmkv/bobhash/bobhash32.cpp -o CMakeFiles/nvmkv-bobhash32.dir/bobhash32.cpp.s

# Object files for target nvmkv-bobhash32
nvmkv__bobhash32_OBJECTS = \
"CMakeFiles/nvmkv-bobhash32.dir/bobhash32.cpp.o"

# External object files for target nvmkv-bobhash32
nvmkv__bobhash32_EXTERNAL_OBJECTS =

bobhash/libnvmkv-bobhash32.a: bobhash/CMakeFiles/nvmkv-bobhash32.dir/bobhash32.cpp.o
bobhash/libnvmkv-bobhash32.a: bobhash/CMakeFiles/nvmkv-bobhash32.dir/build.make
bobhash/libnvmkv-bobhash32.a: bobhash/CMakeFiles/nvmkv-bobhash32.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libnvmkv-bobhash32.a"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bobhash && $(CMAKE_COMMAND) -P CMakeFiles/nvmkv-bobhash32.dir/cmake_clean_target.cmake
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bobhash && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/nvmkv-bobhash32.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
bobhash/CMakeFiles/nvmkv-bobhash32.dir/build: bobhash/libnvmkv-bobhash32.a

.PHONY : bobhash/CMakeFiles/nvmkv-bobhash32.dir/build

bobhash/CMakeFiles/nvmkv-bobhash32.dir/clean:
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bobhash && $(CMAKE_COMMAND) -P CMakeFiles/nvmkv-bobhash32.dir/cmake_clean.cmake
.PHONY : bobhash/CMakeFiles/nvmkv-bobhash32.dir/clean

bobhash/CMakeFiles/nvmkv-bobhash32.dir/depend:
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/wangke/CLionProjects/nvmkv /Users/wangke/CLionProjects/nvmkv/bobhash /Users/wangke/CLionProjects/nvmkv/cmake-build-debug /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bobhash /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/bobhash/CMakeFiles/nvmkv-bobhash32.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : bobhash/CMakeFiles/nvmkv-bobhash32.dir/depend

