# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

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
CMAKE_SOURCE_DIR = /home/sudarina/CLionProjects/pa4-2

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/sudarina/CLionProjects/pa4-2/build

# Include any dependencies generated for this target.
include CMakeFiles/pa3.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/pa3.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/pa3.dir/flags.make

CMakeFiles/pa3.dir/pa23.c.o: CMakeFiles/pa3.dir/flags.make
CMakeFiles/pa3.dir/pa23.c.o: ../pa23.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sudarina/CLionProjects/pa4-2/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/pa3.dir/pa23.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/pa3.dir/pa23.c.o   -c /home/sudarina/CLionProjects/pa4-2/pa23.c

CMakeFiles/pa3.dir/pa23.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/pa3.dir/pa23.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/sudarina/CLionProjects/pa4-2/pa23.c > CMakeFiles/pa3.dir/pa23.c.i

CMakeFiles/pa3.dir/pa23.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/pa3.dir/pa23.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/sudarina/CLionProjects/pa4-2/pa23.c -o CMakeFiles/pa3.dir/pa23.c.s

CMakeFiles/pa3.dir/bank_robbery.c.o: CMakeFiles/pa3.dir/flags.make
CMakeFiles/pa3.dir/bank_robbery.c.o: ../bank_robbery.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sudarina/CLionProjects/pa4-2/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/pa3.dir/bank_robbery.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/pa3.dir/bank_robbery.c.o   -c /home/sudarina/CLionProjects/pa4-2/bank_robbery.c

CMakeFiles/pa3.dir/bank_robbery.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/pa3.dir/bank_robbery.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/sudarina/CLionProjects/pa4-2/bank_robbery.c > CMakeFiles/pa3.dir/bank_robbery.c.i

CMakeFiles/pa3.dir/bank_robbery.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/pa3.dir/bank_robbery.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/sudarina/CLionProjects/pa4-2/bank_robbery.c -o CMakeFiles/pa3.dir/bank_robbery.c.s

# Object files for target pa3
pa3_OBJECTS = \
"CMakeFiles/pa3.dir/pa23.c.o" \
"CMakeFiles/pa3.dir/bank_robbery.c.o"

# External object files for target pa3
pa3_EXTERNAL_OBJECTS =

pa3: CMakeFiles/pa3.dir/pa23.c.o
pa3: CMakeFiles/pa3.dir/bank_robbery.c.o
pa3: CMakeFiles/pa3.dir/build.make
pa3: ../libruntime.so
pa3: CMakeFiles/pa3.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/sudarina/CLionProjects/pa4-2/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable pa3"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/pa3.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/pa3.dir/build: pa3

.PHONY : CMakeFiles/pa3.dir/build

CMakeFiles/pa3.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/pa3.dir/cmake_clean.cmake
.PHONY : CMakeFiles/pa3.dir/clean

CMakeFiles/pa3.dir/depend:
	cd /home/sudarina/CLionProjects/pa4-2/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/sudarina/CLionProjects/pa4-2 /home/sudarina/CLionProjects/pa4-2 /home/sudarina/CLionProjects/pa4-2/build /home/sudarina/CLionProjects/pa4-2/build /home/sudarina/CLionProjects/pa4-2/build/CMakeFiles/pa3.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/pa3.dir/depend

