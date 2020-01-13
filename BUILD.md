# Building and Testing `libpktdecoder`

## Requirements
1. A **C++-11**-compatible compliler
2. `cmake` (min version 3.0)
3. `make`

Developed and tested on **Ubuntu 18.04 (Bionic Beaver)**, with the following
- `g++ (Ubuntu 7.4.0-1ubuntu1~18.04.1) 7.4.0`
- `cmake version 3.10.2`
- `GNU Make 4.1`

The unit-test framework is implemented with [Catch2](https://github.com/catchorg/Catch2).

## Building
After cloning the repository:
1. `cmake .`
2. `make`
3. `make test`

To see more verbose unit test output, run the following from the command line:
`./test/pktDecoderTest --success`

Afterward you can run `./src/example` to verify the library is usable.
