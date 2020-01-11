cmake_minimum_required( VERSION 3.10 )
# Fix behavior of CMAKE_CXX_STANDARD when targeting macOS.
if ( POLICY CMP0025 )
   cmake_policy( SET CMP0025 NEW )
endif ()

project( pktDecoder )

set( PROJECT_LINK_LIBS libpktDecoder.so )
link_directories( ../libsrc )

include_directories(
      ${CMAKE_SOURCE_DIR},
      ../libsrc )

set( SOURCES test_pkt_decoder.cpp )
set( HEADERS catch.hpp )

add_executable( pktDecoderTest test_pkt_decoder.cpp )
target_link_libraries( pktDecoderTest ${PROJECT_LINK_LIBS} )
