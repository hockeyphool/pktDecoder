cmake_minimum_required( VERSION 3.0 )
# Fix behavior of CMAKE_CXX_STANDARD when targeting macOS.
if ( POLICY CMP0025 )
   cmake_policy( SET CMP0025 NEW )
endif ()

project( pktDecoder )

include_directories(
      ${TEST_SOURCE_DIR}/libsrc
      ${CMAKE_SOURCE_DIR} )

set( SOURCES example.cpp )
add_executable( example example.cpp )
target_link_libraries( example pktdecoder )
