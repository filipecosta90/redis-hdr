# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

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
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.15.0/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.15.0/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/filipeoliveria/redislabs/redis_hdr

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/filipeoliveria/redislabs/redis_hdr/build

# Include any dependencies generated for this target.
include CMakeFiles/redis_hdr.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/redis_hdr.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/redis_hdr.dir/flags.make

# Object files for target redis_hdr
redis_hdr_OBJECTS =

# External object files for target redis_hdr
redis_hdr_EXTERNAL_OBJECTS = \
"/Users/filipeoliveria/redislabs/redis_hdr/build/src/CMakeFiles/redis_hdr_obj.dir/redis_hdr.c.o" \
"/Users/filipeoliveria/redislabs/redis_hdr/build/src/CMakeFiles/redis_hdr_obj.dir/hdr_histogram.c.o" \
"/Users/filipeoliveria/redislabs/redis_hdr/build/src/CMakeFiles/redis_hdr_obj.dir/hdr_histogram_log.c.o"

redis_hdr.so: src/CMakeFiles/redis_hdr_obj.dir/redis_hdr.c.o
redis_hdr.so: src/CMakeFiles/redis_hdr_obj.dir/hdr_histogram.c.o
redis_hdr.so: src/CMakeFiles/redis_hdr_obj.dir/hdr_histogram_log.c.o
redis_hdr.so: CMakeFiles/redis_hdr.dir/build.make
redis_hdr.so: /usr/lib/libz.dylib
redis_hdr.so: CMakeFiles/redis_hdr.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/filipeoliveria/redislabs/redis_hdr/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Linking C shared library redis_hdr.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/redis_hdr.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/redis_hdr.dir/build: redis_hdr.so

.PHONY : CMakeFiles/redis_hdr.dir/build

CMakeFiles/redis_hdr.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/redis_hdr.dir/cmake_clean.cmake
.PHONY : CMakeFiles/redis_hdr.dir/clean

CMakeFiles/redis_hdr.dir/depend:
	cd /Users/filipeoliveria/redislabs/redis_hdr/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/filipeoliveria/redislabs/redis_hdr /Users/filipeoliveria/redislabs/redis_hdr /Users/filipeoliveria/redislabs/redis_hdr/build /Users/filipeoliveria/redislabs/redis_hdr/build /Users/filipeoliveria/redislabs/redis_hdr/build/CMakeFiles/redis_hdr.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/redis_hdr.dir/depend

