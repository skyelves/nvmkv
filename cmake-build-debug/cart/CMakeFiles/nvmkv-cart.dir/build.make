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
include cart/CMakeFiles/nvmkv-cart.dir/depend.make

# Include the progress variables for this target.
include cart/CMakeFiles/nvmkv-cart.dir/progress.make

# Include the compile flags for this target's objects.
include cart/CMakeFiles/nvmkv-cart.dir/flags.make

cart/CMakeFiles/nvmkv-cart.dir/cart.cpp.o: cart/CMakeFiles/nvmkv-cart.dir/flags.make
cart/CMakeFiles/nvmkv-cart.dir/cart.cpp.o: ../cart/cart.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object cart/CMakeFiles/nvmkv-cart.dir/cart.cpp.o"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/nvmkv-cart.dir/cart.cpp.o -c /Users/wangke/CLionProjects/nvmkv/cart/cart.cpp

cart/CMakeFiles/nvmkv-cart.dir/cart.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/nvmkv-cart.dir/cart.cpp.i"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/wangke/CLionProjects/nvmkv/cart/cart.cpp > CMakeFiles/nvmkv-cart.dir/cart.cpp.i

cart/CMakeFiles/nvmkv-cart.dir/cart.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/nvmkv-cart.dir/cart.cpp.s"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/wangke/CLionProjects/nvmkv/cart/cart.cpp -o CMakeFiles/nvmkv-cart.dir/cart.cpp.s

cart/CMakeFiles/nvmkv-cart.dir/cart_node.cpp.o: cart/CMakeFiles/nvmkv-cart.dir/flags.make
cart/CMakeFiles/nvmkv-cart.dir/cart_node.cpp.o: ../cart/cart_node.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object cart/CMakeFiles/nvmkv-cart.dir/cart_node.cpp.o"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/nvmkv-cart.dir/cart_node.cpp.o -c /Users/wangke/CLionProjects/nvmkv/cart/cart_node.cpp

cart/CMakeFiles/nvmkv-cart.dir/cart_node.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/nvmkv-cart.dir/cart_node.cpp.i"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/wangke/CLionProjects/nvmkv/cart/cart_node.cpp > CMakeFiles/nvmkv-cart.dir/cart_node.cpp.i

cart/CMakeFiles/nvmkv-cart.dir/cart_node.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/nvmkv-cart.dir/cart_node.cpp.s"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/wangke/CLionProjects/nvmkv/cart/cart_node.cpp -o CMakeFiles/nvmkv-cart.dir/cart_node.cpp.s

# Object files for target nvmkv-cart
nvmkv__cart_OBJECTS = \
"CMakeFiles/nvmkv-cart.dir/cart.cpp.o" \
"CMakeFiles/nvmkv-cart.dir/cart_node.cpp.o"

# External object files for target nvmkv-cart
nvmkv__cart_EXTERNAL_OBJECTS =

cart/libnvmkv-cart.a: cart/CMakeFiles/nvmkv-cart.dir/cart.cpp.o
cart/libnvmkv-cart.a: cart/CMakeFiles/nvmkv-cart.dir/cart_node.cpp.o
cart/libnvmkv-cart.a: cart/CMakeFiles/nvmkv-cart.dir/build.make
cart/libnvmkv-cart.a: cart/CMakeFiles/nvmkv-cart.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/wangke/CLionProjects/nvmkv/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library libnvmkv-cart.a"
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && $(CMAKE_COMMAND) -P CMakeFiles/nvmkv-cart.dir/cmake_clean_target.cmake
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/nvmkv-cart.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
cart/CMakeFiles/nvmkv-cart.dir/build: cart/libnvmkv-cart.a

.PHONY : cart/CMakeFiles/nvmkv-cart.dir/build

cart/CMakeFiles/nvmkv-cart.dir/clean:
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart && $(CMAKE_COMMAND) -P CMakeFiles/nvmkv-cart.dir/cmake_clean.cmake
.PHONY : cart/CMakeFiles/nvmkv-cart.dir/clean

cart/CMakeFiles/nvmkv-cart.dir/depend:
	cd /Users/wangke/CLionProjects/nvmkv/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/wangke/CLionProjects/nvmkv /Users/wangke/CLionProjects/nvmkv/cart /Users/wangke/CLionProjects/nvmkv/cmake-build-debug /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart /Users/wangke/CLionProjects/nvmkv/cmake-build-debug/cart/CMakeFiles/nvmkv-cart.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : cart/CMakeFiles/nvmkv-cart.dir/depend

