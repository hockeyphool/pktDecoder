cmake_minimum_required( VERSION 3.0 )
# Fix behavior of CMAKE_CXX_STANDARD when targeting macOS.
if ( POLICY CMP0025 )
   cmake_policy( SET CMP0025 NEW )
endif ()
project( pktDecoder )

set( SOURCES pkt_decoder.cpp )
set( HEADERS pkt_decoder.h )

include_directories( ${CMAKE_SOURCE_DIR} )

add_library( pktdecoder ${SOURCES} ${HEADERS} )

target_link_libraries( pktdecoder )

install( TARGETS pktdecoder
      CONFIGURATIONS Release
      DESTINATION /tmp/lib
      LIBRARY
      COMPONENT library )

install( FILES pkt_decoder.h
      DESTINATION /tmp/include )
