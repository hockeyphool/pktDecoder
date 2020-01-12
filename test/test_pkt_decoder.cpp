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

TEST_CASE( "Writer creates a return buffer", "[pktDecoder]" )
{
   const uint8_t expectedValue = 0x04;
   const uint8_t testpkt[] = { STX, expectedValue, ETX };
   pkt_decoder_t* decoder = pkt_decoder_create( nullptr, nullptr );
   pkt_decoder_write_bytes( decoder, sizeof( testpkt ), testpkt );
   REQUIRE( expectedValue == *( decoder->m_pktByteList ) );
   pkt_decoder_destroy( decoder );
}

TEST_CASE( "See if we get a warning", "[pktDecoder]" )
{
   const uint8_t expectedValue = 0x04;
   const uint8_t testpkt[] = { STX, 0x05, 0x06, STX, 0x04, ETX };
   pkt_decoder_t* decoder = pkt_decoder_create( nullptr, nullptr );
   pkt_decoder_write_bytes( decoder, sizeof( testpkt ), testpkt );
   REQUIRE( expectedValue == *( decoder->m_pktByteList ) );
   pkt_decoder_destroy( decoder );
}
