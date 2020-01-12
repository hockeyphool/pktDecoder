#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <libsrc/pkt_decoder.h>

TEST_CASE( "Create a valid packet decoder", "[pktDecoder]" )
{
   pkt_decoder_t* decoder = pkt_decoder_create( nullptr, nullptr );
   REQUIRE( nullptr != decoder );
}

TEST_CASE( "Destroy a packet decoder", "[pktDecoder]" )
{
   pkt_decoder_t* decoder = pkt_decoder_create( nullptr, nullptr );
   REQUIRE_NOTHROW( pkt_decoder_destroy( decoder ) );
}
