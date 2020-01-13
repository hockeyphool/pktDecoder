cmake_minimum_required( VERSION 3.0 )
project( pktDecoder )

set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

add_subdirectory( libsrc )
add_subdirectory( src )

enable_testing()
add_subdirectory( test )
