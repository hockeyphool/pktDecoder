#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <cstring>
#include <libsrc/pkt_decoder.h>

using std::cout;
using std::endl;
using std::ostringstream;
using std::strncmp;

// Test state variables
bool callbackWasCalled( false );
u_int8_t numCallbacks( 0 );
size_t actualBufferLength( 0 );

// Callback function to set state variables for validation
static void myCallbackFunc( void* ctx, size_t bufferLength, const uint8_t* dataBuffer )
{
   ( void )ctx;
   ( void )dataBuffer;
   actualBufferLength = bufferLength;
   numCallbacks++;
   callbackWasCalled = true;
}

// This listener will ensure that state variables are reset before another section runs
struct CleanupListener : Catch::TestEventListenerBase
{
   using TestEventListenerBase::TestEventListenerBase;
   void sectionEnded( Catch::SectionStats const& sectionStats ) override
   {
      callbackWasCalled = false;
      numCallbacks = 0;
      actualBufferLength = 0;
   }
};
CATCH_REGISTER_LISTENER( CleanupListener );

TEST_CASE( "Validate packet decoder construction & destruction", "[management]" )
{
   SECTION( "Create a packet decoder" )
   {
      pkt_decoder_t* decoder = pkt_decoder_create( nullptr, nullptr );
      REQUIRE( nullptr != decoder );
      pkt_decoder_destroy( decoder );
   }

   SECTION( "Destroy a packet decoder" )
   {
      pkt_decoder_t* decoder = pkt_decoder_create( nullptr, nullptr );
      REQUIRE_NOTHROW( pkt_decoder_destroy( decoder ) );

      // Catch2 ought to provide a way to handle the follow assert, which terminates with a SIGSEGV.
      // This would allow verification that the previous destruction succeeded. This will
      // require following up with the Catch2 community for assistance.
      // REQUIRE_THROWS(pkt_decoder_destroy( decoder );
   }
}

TEST_CASE( "Verify packet decoder internals", "[internals]" )
{
   SECTION( "Verify internal variables - simple packet" )
   {
      const uint8_t EXP_VALUE = 0x04;
      const uint8_t BYTESTREAM[] = { STX, EXP_VALUE, ETX };
      pkt_decoder_t* decoder = pkt_decoder_create( nullptr, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM ), BYTESTREAM );
      REQUIRE( 1 == decoder->m_pktBufIdx );
      REQUIRE( EXP_VALUE == decoder->m_packetBuffer[ 0 ] );
      pkt_decoder_destroy( decoder );
   }

   SECTION( "Verify internal variable changes - multi-stream writes" )
   {
      const uint8_t BYTESTREAM1[] = { STX, 0x04, 0x05, 0x06 };
      const uint8_t BYTESTREAM2[] = { 0x07, DLE, 0x30, 0x08 };
      const uint8_t BYTESTREAM3[] = { 0x09, ETX };
      pkt_decoder_t* decoder = pkt_decoder_create( nullptr, nullptr );

      // write bytestream 1 and check state
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM1 ), BYTESTREAM1 );
      REQUIRE( decoder->m_pktValid );
      REQUIRE( 3 == decoder->m_pktBufIdx );

      // write bytestream 2 and check state
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM2 ), BYTESTREAM2 );
      REQUIRE( decoder->m_pktValid );
      REQUIRE( 6 == decoder->m_pktBufIdx );

      // write bytestream 3 and check state
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM3 ), BYTESTREAM3 );
      REQUIRE( !decoder->m_pktValid );
      REQUIRE( 7 == decoder->m_pktBufIdx );

      pkt_decoder_destroy( decoder );
   }
}

TEST_CASE( "Validate packet decoding & callbacks - Valid packets", "[validation]" )
{
   SECTION( "Simple two-byte packet" )
   {
      REQUIRE_FALSE( callbackWasCalled );
      const uint8_t BYTESTREAM[] = { STX, 0x4f, 0x4b, ETX };
      const uint8_t EXPECTED_VALUE[] = { 'O', 'K' };
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM ), BYTESTREAM );
      REQUIRE( callbackWasCalled );
      REQUIRE( numCallbacks == 1 );
      REQUIRE( actualBufferLength == sizeof( EXPECTED_VALUE ) );
      REQUIRE( strncmp( ( char* )EXPECTED_VALUE, ( char* )decoder->m_packetBuffer, 2 ) == 0 );
      pkt_decoder_destroy( decoder );
   }

   SECTION( "Simple byte-stuffed packet" )
   {
      REQUIRE_FALSE( callbackWasCalled );
      const uint8_t BYTE_STUFFED_VAL( DLE | ENC );
      const uint8_t BYTESTREAM[] = { STX, DLE, BYTE_STUFFED_VAL, ETX };
      const uint8_t EXPECTED_VALUE( DLE );

      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM ), BYTESTREAM );
      REQUIRE( callbackWasCalled );
      REQUIRE( numCallbacks == 1 );
      REQUIRE( actualBufferLength == sizeof( EXPECTED_VALUE ) );
      REQUIRE( EXPECTED_VALUE == decoder->m_packetBuffer[ 0 ] );

      pkt_decoder_destroy( decoder );
   }

   SECTION( "Verify a packet that spans writes" )
   {
      REQUIRE_FALSE( callbackWasCalled );
      const uint8_t BYTESTREAM_START[] = { STX, 0x53, 0x63 };
      const uint8_t BYTESTREAM_END[] = { 0x6f, 0x74, 0x74, ETX };
      const uint8_t EXPECTED_VAL[] = { 'S', 'c', 'o', 't', 't' };

      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM_START ), BYTESTREAM_START );
      REQUIRE( decoder->m_pktValid );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM_END ), BYTESTREAM_END );
      REQUIRE( callbackWasCalled );
      REQUIRE( numCallbacks == 1 );
      REQUIRE( actualBufferLength == sizeof( EXPECTED_VAL ) );
      REQUIRE(
         strncmp( ( char* )EXPECTED_VAL, ( char* )decoder->m_packetBuffer, sizeof( EXPECTED_VAL ) )
         == 0 );

      pkt_decoder_destroy( decoder );
   }

   SECTION( "Verify a max-size packet is handled" )
   {
      // Note that we can't send 0x02, 0x03, or 0x10 explicitly. Not worth adding DLE & byte-stuffed
      // values
      const uint8_t LARGE_FRAME[] = { 0x00, 0x01, 0x01, 0x01, 0x04, 0x05, 0x06, 0x07,
                                      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
      const size_t MAX_FRAMES( MAX_DECODED_DATA_LENGTH / sizeof( LARGE_FRAME ) );

      REQUIRE_FALSE( callbackWasCalled );
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );

      // start packet decoding
      pkt_decoder_write_bytes( decoder, 1, &STX );

      // write to limit and check
      for ( size_t idx = 0; idx < MAX_FRAMES; ++idx )
      {
         size_t exp_size( sizeof( LARGE_FRAME ) * ( idx + 1 ) );
         pkt_decoder_write_bytes( decoder, sizeof( LARGE_FRAME ), LARGE_FRAME );
         REQUIRE( exp_size == decoder->m_pktBufIdx );
         REQUIRE( decoder->m_pktValid );
      }

      // Finish the packet
      pkt_decoder_write_bytes( decoder, sizeof( ETX ), &ETX );
      REQUIRE( callbackWasCalled );
      REQUIRE( numCallbacks == 1 );
      REQUIRE( actualBufferLength == MAX_DECODED_DATA_LENGTH );

      pkt_decoder_destroy( decoder );
   }

   SECTION( "Verify extraneous bytes are discarded" )
   {
      const uint8_t BYTESTREAM_START[] = { 0x01, 0x04, STX, 0x01, 0x04 };
      const uint8_t BYTESTREAM_END[] = { 0x05, 0x06, DLE, 0x30, ETX, 0x07, 0x08, 0x09 };
      const uint8_t EXPECTED_VALUE[] = { 0x01, 0x04, 0x05, 0x06, DLE };

      REQUIRE_FALSE( callbackWasCalled );
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM_START ), BYTESTREAM_START );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM_END ), BYTESTREAM_END );
      REQUIRE( callbackWasCalled );
      REQUIRE( numCallbacks == 1 );
      REQUIRE( actualBufferLength == sizeof( EXPECTED_VALUE ) );
      REQUIRE( strncmp( ( char* )EXPECTED_VALUE,
                        ( char* )decoder->m_packetBuffer,
                        sizeof( EXPECTED_VALUE ) )
               == 0 );

      pkt_decoder_destroy( decoder );
   }

   SECTION( "Verify multiple packets in one byte stream are all handled" )
   {
      const uint8_t BYTESTREAM[] = { STX, 0x01, ETX, STX, 0x04, ETX, STX, 0x05, ETX };

      REQUIRE_FALSE( callbackWasCalled );
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM ), BYTESTREAM );
      REQUIRE( callbackWasCalled );
      REQUIRE( numCallbacks == 3 );

      pkt_decoder_destroy( decoder );
   }

   SECTION( "Verify that every possible byte value is handled" )
   {
      const size_t NUM_VALUES( 0xFF + 1 /* account for 0 */ + 3 /* account for DLEs */ );
      uint8_t BYTESTREAM[ NUM_VALUES ] = {};

      uint8_t byteVal( 0 );
      ostringstream oss;

      for ( size_t idx = 0; idx < NUM_VALUES; ++idx )
      {
         if ( STX == byteVal || ETX == byteVal || DLE == byteVal )
         {
            // Insert DLE and byte-stuff the actual value. Don't forget to increment the index!
            BYTESTREAM[ idx++ ] = DLE;
            BYTESTREAM[ idx ] = ( byteVal | ENC );
         }
         else
         {
            BYTESTREAM[ idx ] = byteVal;
         }
         byteVal++;
      }

      REQUIRE_FALSE( callbackWasCalled );
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( STX ), &STX );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM ), BYTESTREAM );
      pkt_decoder_write_bytes( decoder, sizeof( ETX ), &ETX );
      REQUIRE( callbackWasCalled );
      REQUIRE( numCallbacks == 1 );
      // Account for the 3 DLE's in the byte stream that are not present in the packet buffer
      REQUIRE( actualBufferLength == ( NUM_VALUES - 3 ) );

      pkt_decoder_destroy( decoder );
   }
}

TEST_CASE( "Validate packet decoding & callbacks - Invalid packets", "[validation]" )
{
   SECTION( "Verify ETX by itself doesn't result in a callback" )
   {
      REQUIRE_FALSE( callbackWasCalled );
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( ETX ), &ETX );
      REQUIRE_FALSE( callbackWasCalled );

      pkt_decoder_destroy( decoder );
   }

   SECTION( "Verify a too-large packet silently fails" )
   {
      // Note that we can't send 0x02, 0x03, or 0x10 explicitly. Not worth adding DLE & byte-stuffed
      // values
      const uint8_t LARGE_FRAME[] = { 0x00, 0x01, 0x01, 0x01, 0x04, 0x05, 0x06, 0x07,
                                      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
      const size_t MAX_FRAMES( MAX_DECODED_DATA_LENGTH / sizeof( LARGE_FRAME ) );

      REQUIRE_FALSE( callbackWasCalled );
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );

      // start packet decoding
      pkt_decoder_write_bytes( decoder, 1, &STX );

      // write to limit
      for ( size_t idx = 0; idx < MAX_FRAMES; ++idx )
      {
         size_t exp_size( sizeof( LARGE_FRAME ) * ( idx + 1 ) );
         pkt_decoder_write_bytes( decoder, sizeof( LARGE_FRAME ), LARGE_FRAME );
      }

      // Confirm we're at the limit and packet is still valid
      REQUIRE( MAX_DECODED_DATA_LENGTH == decoder->m_pktBufIdx );
      REQUIRE( decoder->m_pktValid );

      // write past limit and verify silent failure
      pkt_decoder_write_bytes( decoder, sizeof( LARGE_FRAME ), LARGE_FRAME );
      REQUIRE( 0 == decoder->m_pktBufIdx );
      REQUIRE_FALSE( decoder->m_pktValid );

      // verify callback isn't called
      pkt_decoder_write_bytes( decoder, 1, &ETX );
      REQUIRE_FALSE( callbackWasCalled );

      pkt_decoder_destroy( decoder );
   }

   SECTION( "Verify empty packet is silently dropped" )
   {
      const uint8_t BYTESTREAM[] = { STX, ETX };

      REQUIRE_FALSE( callbackWasCalled );
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM ), BYTESTREAM );
      REQUIRE_FALSE( callbackWasCalled );

      pkt_decoder_destroy( decoder );
   }

   SECTION( "Verify incomplete packet is dropped and valid packet is handled" )
   {
      const uint8_t BYTESTREAM[] = { STX, 0x01, 0x02, STX, 0x04, 0x05, ETX };
      const uint8_t EXPECTED_VALUE[] = { 0x04, 0x05 };

      REQUIRE_FALSE( callbackWasCalled );
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM ), BYTESTREAM );
      REQUIRE( callbackWasCalled );
      REQUIRE( numCallbacks == 1 );
      REQUIRE( actualBufferLength == sizeof( EXPECTED_VALUE ) );
      REQUIRE( strncmp( ( char* )EXPECTED_VALUE,
                        ( char* )decoder->m_packetBuffer,
                        sizeof( EXPECTED_VALUE ) )
               == 0 );

      pkt_decoder_destroy( decoder );
   }
}
