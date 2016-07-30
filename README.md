# Read Me for Dependency Checker

Copyright (C) 2012-2016, TomTom International BV. All rights reserved.
----

The tool `cpp-dependencies` creates `#include` dependency information for C++ source
code files, which it derives from scanning a full source tree.

The dependency information is output as `.dot` files, which can be visualized
in, for example, [GraphViz](http://www.graphviz.org).

Happy coding!

Peter Bindels and Rijn Buve

*TomTom International BV*

# License

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

# Build and run

The tool depends on Boost.Filesystem being available and usable. Installing this
should be done with your platform's package management system, such as Apt, 
Pacman or Brew.

To build the tool, execute:

    make

This create the executable file `cpp-dependencies`.

To clean up the files created by the build, execute:

    make clean
    
To check if the tool was compiled correctly, execute:
  
    ./cpp-dependencies
    
This provides help information about the tool. More information about
its usage is presented in the next paragraph.
    
    
# Using `cpp-dependencies` to analyze a component

As a first thing on a code base is to find out whether it can read the code correctly. From the root of the project, 
run the command:

    cpp-dependencies --stats .

to determine the complexity of the code base and the amount of nodes that are entangled in cycles with other 
components. In well set-up projects, the cycle count will be equal to zero and the amount of components will be in 
the same order of size as the logical components you expect.

To investigate a specific component, you can use 

    cpp-dependencies --info <component> .
    
for all information the tool has on the component, or:    

    cpp-dependencies --inout <component> .
    
to find out who links to and from your component.

In case you have a dependency that you were not expecting, or find out that when rebuilding component A that a
supposedly-unrelated component B is built, you can use:

    cpp-dependencies --shortest A B .

to determine why there is a link from component A to component B. It will find one of the shortest paths it can find 
from A to B if there is one.


# Using `cpp-dependencies` to make visualized graphs

The tool is also able to provide output in `.dot` format, which is a format used by [GraphViz](http://www.graphviz.org) and other tools to contain
graphs. It is a human-readable format and can be used by the dot tool to convert it into a graphical image. To create
a graph file, use:

    cpp-dependencies --graph mygraph.dot .

to create a mygraph.dot file containing the full component graph. 

You can restrict the component graph to either all
components beneath a given target (`--graph-for <output> <target>`) or all components part of a cycle (`--graph-cycles`). 

To make this text-format graph into a viewable graph, use for example:

    dot -Tpng mygraph.dot >mygraph.png

to convert it into a PNG file. 

The `dot` program will try to find a way to graphically display the graph output. Note that
very large graphs, in particular if many cycles are present, can take hours to render.


# Editing the tool

The tool itself is split up into a few separate files to make it easier to find and extend its functionality. The following files are found:

* `main.cpp` contains the main functions and help information, as well as the core flow.
* `Input.cpp` contains the functions that read C++ and `CMakeLists` files into the information needed by the tool.
* `Output.cpp` contains functions to write all output files generated, except for the `CMakeLists` generation.
* `CmakeRegen.cpp` contains the functionality to write `CMakeLists` files.
* `Analysis.cpp` contains all graph processing and navigation functions.
* `Component.cpp` contains the implementation needed for the struct-like data storage classes.
* `generated.cpp` contains the function to convert found header files into a lookup map. Also the place to add generated files
    to the known file list, so that they will be taken into account for components.
* `Constants.h` contains the constants used throughout the code base.

In general, the root functionality is kept in `main.cpp`, the structural classes are kept in `Component.cpp` 
and any auxiliary functions that are used to do this are split up by domain.


# Rationale behind implementation

The tool was implemented with the goal of being able to quickly analyze dependencies between components of a 
complex project, including how the dependency graph changes when some changes are made to the source tree. To 
accomplish this, choices were made in the direction of more performance at the expense of strict correctness. 
Specifically:

- It does not use a proper C++ parser to read C++ files, nor a proper CMake parser to read CMake files. Properly
  parsing these files would increase the full run time of the program by orders of magnitude and make it much less
  useful.
- `strstr` is used across the full code base. While profiling, we found that `std::string::find` was taking over 80% of
  the full runtime. Replacing it with `strstr`, which is typically much more optimized, made the whole program twice 
  as fast.

This results in it running on a 1.5GB source code base in about 2.1 seconds -- fast enough for interactive checks and
rerunning after any small modification. 

The tool was set up to compile on a Ubuntu 12.04 system with the platform default compiler. This means that the sources will use C++11
but will not use anything not available in GCC 4.6. It has been tested and used on Linux (Ubuntu 12.04 - 16.04) and MacOS X 
(different versions).


# Using Git and `.gitignore`

It's good practice to set up a personal global `.gitignore` file on your machine which filters a number of files
on your file systems that you do not wish to submit to the Git repository. You can set up your own global
`~/.gitignore` file by executing:
`git config --global core.excludesfile ~/.gitignore`

In general, add the following file types to `~/.gitignore` (each entry should be on a separate line):
`*.com *.class *.dll *.exe *.o *.so *.log *.sql *.sqlite *.tlog *.epoch *.swp *.hprof *.hprof.index *.releaseBackup *~`

The local `.gitignore` file in the Git repository itself to reflect those file only that are produced by executing
regular compile, build or release commands.


# Bug reports and new feature requests

If you encounter any problems with this library, don't hesitate to use the `Issues` session to file your issues.
Normally, one of our developers should be able to comment on them and fix.
