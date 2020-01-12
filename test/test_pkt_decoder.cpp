#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <cstring>
#include <libsrc/pkt_decoder.h>

using std::cout;
using std::endl;
using std::ostringstream;
using std::string;
using std::strncmp;

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

      REQUIRE_NOTHROW( pkt_decoder_destroy( decoder ) );
   }
}

bool callbackCalled( false );
static void myCallbackFunc( void* ctx, size_t bufferLength, const uint8_t* dataBuffer )
{
   ( void )ctx;
   ostringstream oss;
   oss << "Received buffer (length = " << bufferLength << ")";
   cout << oss.str() << endl;
   callbackCalled = true;
}

TEST_CASE( "Validate packet decoding & callbacks", "[validation]" )
{
   SECTION( "Verify ETX by itself doesn't result in a callback" )
   {
      REQUIRE( !callbackCalled );
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( ETX ), &ETX );
      REQUIRE( !callbackCalled );

      pkt_decoder_destroy( decoder );
   }

   SECTION( "Simple two-byte packet" )
   {
      REQUIRE( !callbackCalled );
      const uint8_t BYTESTREAM[] = { STX, 0x4f, 0x4b, ETX };
      ostringstream expVal;
      expVal << "OK";
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM ), BYTESTREAM );
      REQUIRE( callbackCalled );
      REQUIRE( strncmp( expVal.str().c_str(), ( char* )decoder->m_packetBuffer, 2 ) == 0 );
      pkt_decoder_destroy( decoder );
      callbackCalled = false;
   }

   SECTION( "Simple byte-stuffed packet" )
   {
      REQUIRE( !callbackCalled );
      const uint8_t BYTE_STUFFED_VAL( DLE | ENC );
      const uint8_t BYTESTREAM[] = { STX, DLE, BYTE_STUFFED_VAL, ETX };
      const uint8_t EXPECTED_VALUE( BYTE_STUFFED_VAL & ~ENC );

      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM ), BYTESTREAM );
      REQUIRE( callbackCalled );
      REQUIRE( EXPECTED_VALUE == decoder->m_packetBuffer[ 0 ] );

      pkt_decoder_destroy( decoder );
      callbackCalled = false;
   }

   SECTION( "Verify a packet that spans writes" )
   {
      REQUIRE( !callbackCalled );
      const uint8_t BYTESTREAM_START[] = { STX, 0x53, 0x63 };
      const uint8_t BYTESTREAM_END[] = { 0x6f, 0x74, 0x74, ETX };
      const string EXPECTED_VAL( "Scott" );

      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM_START ), BYTESTREAM_START );
      REQUIRE( decoder->m_pktValid );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM_END ), BYTESTREAM_END );
      REQUIRE( callbackCalled );
      REQUIRE(
         strncmp( EXPECTED_VAL.c_str(), ( char* )decoder->m_packetBuffer, EXPECTED_VAL.size() )
         == 0 );

      pkt_decoder_destroy( decoder );
      callbackCalled = false;
   }

   SECTION( "Verify a max-size packet is handled" )
   {
      // Note that we can't send 0x02 or 0x03 explicitly. Not worth adding DLE & byte-stuffed values
      const uint8_t LARGE_FRAME[] = { 0x00, 0x01, 0x01, 0x01, 0x04, 0x05, 0x06, 0x07,
                                      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
      const size_t MAX_FRAMES( MAX_DECODED_DATA_LENGTH / sizeof( LARGE_FRAME ) );

      REQUIRE( !callbackCalled );
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
      REQUIRE( callbackCalled );

      // Confirm we're at the limit
      REQUIRE( MAX_DECODED_DATA_LENGTH == decoder->m_pktBufIdx );

      pkt_decoder_destroy( decoder );
      callbackCalled = false;
   }

   SECTION( "Verify a too-large packet silently fails" )
   {
      // Note that we can't send 0x02 or 0x03 explicitly. Not worth adding DLE & byte-stuffed values
      const uint8_t LARGE_FRAME[] = { 0x00, 0x01, 0x01, 0x01, 0x04, 0x05, 0x06, 0x07,
                                      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
      const size_t MAX_FRAMES( MAX_DECODED_DATA_LENGTH / sizeof( LARGE_FRAME ) );

      REQUIRE( !callbackCalled );
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

      // Confirm we're at the limit
      REQUIRE( MAX_DECODED_DATA_LENGTH == decoder->m_pktBufIdx );

      // write past limit and verify silent failure
      pkt_decoder_write_bytes( decoder, sizeof( LARGE_FRAME ), LARGE_FRAME );
      REQUIRE( 0 == decoder->m_pktBufIdx );

      // verify callback isn't called
      pkt_decoder_write_bytes( decoder, 1, &ETX );
      REQUIRE( !callbackCalled );

      pkt_decoder_destroy( decoder );
   }

   SECTION( "Verify empty packet is silently dropped" )
   {
      const uint8_t BYTESTREAM[] = { STX, ETX };

      REQUIRE( !callbackCalled );
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM ), BYTESTREAM );
      REQUIRE( !callbackCalled );

      pkt_decoder_destroy( decoder );
   }

   SECTION( "Verify extraneous bytes are discarded" )
   {
      const uint8_t BYTESTREAM_START[] = { 0x01, 0x04, STX, 0x01, 0x04 };
      const uint8_t BYTESTREAM_END[] = { 0x05, 0x06, DLE, 0x30, ETX, 0x07, 0x08, 0x09 };
      const uint8_t EXPECTED_VALUE[] = { 0x01, 0x04, 0x05, 0x06, DLE };

      REQUIRE( !callbackCalled );
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM_START ), BYTESTREAM_START );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM_END ), BYTESTREAM_END );
      REQUIRE( callbackCalled );
      REQUIRE( sizeof( EXPECTED_VALUE ) == decoder->m_pktBufIdx );
      REQUIRE( strncmp( ( char* )EXPECTED_VALUE,
                        ( char* )decoder->m_packetBuffer,
                        sizeof( EXPECTED_VALUE ) )
               == 0 );

      pkt_decoder_destroy( decoder );
      callbackCalled = false;
   }

   SECTION( "Verify incomplete packet is dropped and valid packet is handled" )
   {
      const uint8_t BYTESTREAM[] = { STX, 0x01, 0x02, STX, 0x04, 0x05, ETX };
      const uint8_t EXPECTED_VALUE[] = { 0x04, 0x05 };

      REQUIRE( !callbackCalled );
      pkt_decoder_t* decoder = pkt_decoder_create( myCallbackFunc, nullptr );
      pkt_decoder_write_bytes( decoder, sizeof( BYTESTREAM ), BYTESTREAM );
      REQUIRE( callbackCalled );
      REQUIRE( sizeof( EXPECTED_VALUE ) == decoder->m_pktBufIdx );
      REQUIRE( strncmp( ( char* )EXPECTED_VALUE,
                        ( char* )decoder->m_packetBuffer,
                        sizeof( EXPECTED_VALUE ) )
               == 0 );

      pkt_decoder_destroy( decoder );
      callbackCalled = false;
   }
}
