# Building and Testing `libpktdecoder`

## Requirements
1. A **C++-11**-compatible compliler
1. `cmake` (min version 3.0)
1. `make`

Developed and tested on **Ubuntu 18.04 (Bionic Beaver)**, with the following
- `g++ (Ubuntu 7.4.0-1ubuntu1~18.04.1) 7.4.0`
- `cmake version 3.10.2`
- `GNU Make 4.1`

The unit-test framework is implemented with [Catch2](https://github.com/catchorg/Catch2).

## Building
After cloning the repository:
1. `cmake .`
1. `make`
1. `make test`

Afterward you can run `./src/example` to verify the library is usable.

## Installation
The library's `CMakeLists.txt` configuration defines a **Release** installation target (default location is `/tmp/lib` for the library and `/tmp/include` for the header). if you wish to install the library:
1. Edit `libsrc/CMakeLists.txt` and change the `install` **DESTINATION**  configuration to the directories you wish to use.
1. `cmake -DCMAKE_BUILD_TYPE=Release .`
1. `make && make test`
1. Either `make install` (if you have permission to write in your installation directory) or `sudo make install` (if your installation directory requires super-user permission to install)
