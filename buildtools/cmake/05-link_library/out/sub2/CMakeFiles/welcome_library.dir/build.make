# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /media/xue/D/cmake/05-link_library

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /media/xue/D/cmake/05-link_library/out

# Include any dependencies generated for this target.
include sub2/CMakeFiles/welcome_library.dir/depend.make

# Include the progress variables for this target.
include sub2/CMakeFiles/welcome_library.dir/progress.make

# Include the compile flags for this target's objects.
include sub2/CMakeFiles/welcome_library.dir/flags.make

sub2/CMakeFiles/welcome_library.dir/welcome.cpp.o: sub2/CMakeFiles/welcome_library.dir/flags.make
sub2/CMakeFiles/welcome_library.dir/welcome.cpp.o: ../sub2/welcome.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/media/xue/D/cmake/05-link_library/out/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object sub2/CMakeFiles/welcome_library.dir/welcome.cpp.o"
	cd /media/xue/D/cmake/05-link_library/out/sub2 && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/welcome_library.dir/welcome.cpp.o -c /media/xue/D/cmake/05-link_library/sub2/welcome.cpp

sub2/CMakeFiles/welcome_library.dir/welcome.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/welcome_library.dir/welcome.cpp.i"
	cd /media/xue/D/cmake/05-link_library/out/sub2 && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /media/xue/D/cmake/05-link_library/sub2/welcome.cpp > CMakeFiles/welcome_library.dir/welcome.cpp.i

sub2/CMakeFiles/welcome_library.dir/welcome.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/welcome_library.dir/welcome.cpp.s"
	cd /media/xue/D/cmake/05-link_library/out/sub2 && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /media/xue/D/cmake/05-link_library/sub2/welcome.cpp -o CMakeFiles/welcome_library.dir/welcome.cpp.s

sub2/CMakeFiles/welcome_library.dir/welcome.cpp.o.requires:

.PHONY : sub2/CMakeFiles/welcome_library.dir/welcome.cpp.o.requires

sub2/CMakeFiles/welcome_library.dir/welcome.cpp.o.provides: sub2/CMakeFiles/welcome_library.dir/welcome.cpp.o.requires
	$(MAKE) -f sub2/CMakeFiles/welcome_library.dir/build.make sub2/CMakeFiles/welcome_library.dir/welcome.cpp.o.provides.build
.PHONY : sub2/CMakeFiles/welcome_library.dir/welcome.cpp.o.provides

sub2/CMakeFiles/welcome_library.dir/welcome.cpp.o.provides.build: sub2/CMakeFiles/welcome_library.dir/welcome.cpp.o


# Object files for target welcome_library
welcome_library_OBJECTS = \
"CMakeFiles/welcome_library.dir/welcome.cpp.o"

# External object files for target welcome_library
welcome_library_EXTERNAL_OBJECTS =

sub2/libwelcome_library.a: sub2/CMakeFiles/welcome_library.dir/welcome.cpp.o
sub2/libwelcome_library.a: sub2/CMakeFiles/welcome_library.dir/build.make
sub2/libwelcome_library.a: sub2/CMakeFiles/welcome_library.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/media/xue/D/cmake/05-link_library/out/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libwelcome_library.a"
	cd /media/xue/D/cmake/05-link_library/out/sub2 && $(CMAKE_COMMAND) -P CMakeFiles/welcome_library.dir/cmake_clean_target.cmake
	cd /media/xue/D/cmake/05-link_library/out/sub2 && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/welcome_library.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
sub2/CMakeFiles/welcome_library.dir/build: sub2/libwelcome_library.a

.PHONY : sub2/CMakeFiles/welcome_library.dir/build

sub2/CMakeFiles/welcome_library.dir/requires: sub2/CMakeFiles/welcome_library.dir/welcome.cpp.o.requires

.PHONY : sub2/CMakeFiles/welcome_library.dir/requires

sub2/CMakeFiles/welcome_library.dir/clean:
	cd /media/xue/D/cmake/05-link_library/out/sub2 && $(CMAKE_COMMAND) -P CMakeFiles/welcome_library.dir/cmake_clean.cmake
.PHONY : sub2/CMakeFiles/welcome_library.dir/clean

sub2/CMakeFiles/welcome_library.dir/depend:
	cd /media/xue/D/cmake/05-link_library/out && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /media/xue/D/cmake/05-link_library /media/xue/D/cmake/05-link_library/sub2 /media/xue/D/cmake/05-link_library/out /media/xue/D/cmake/05-link_library/out/sub2 /media/xue/D/cmake/05-link_library/out/sub2/CMakeFiles/welcome_library.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : sub2/CMakeFiles/welcome_library.dir/depend
