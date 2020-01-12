cmake_minimum_required( VERSION 3.10 )
# Fix behavior of CMAKE_CXX_STANDARD when targeting macOS.
if ( POLICY CMP0025 )
   cmake_policy( SET CMP0025 NEW )
endif ()

project( pktDecoder )

include_directories(
      ${TEST_SOURCE_DIR}/libsrc
      ${CMAKE_SOURCE_DIR} )

set( SOURCES test_pkt_decoder.cpp )
set( HEADERS catch.hpp )

add_executable( pktDecoderTest test_pkt_decoder.cpp )
target_link_libraries(
      pktDecoderTest
      pktdecoder )
add_test( NAME PktDecoder COMMAND pktDecoderTest )
