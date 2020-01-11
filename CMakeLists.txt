cmake_minimum_required( VERSION 3.10 )
# Fix behavior of CMAKE_CXX_STANDARD when targeting macOS.
if ( POLICY CMP0025 )
   cmake_policy( SET CMP0025 NEW )
endif ()
project( pktDecoder )

set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

add_library( pktDecoder pkt_decoder.cpp pkt_decoder.h )
