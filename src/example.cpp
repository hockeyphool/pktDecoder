#include <cstdint>
#include <cstdio>
#include <libsrc/pkt_decoder.h>

static void pkt_printer( void* ctx, size_t data_length, const uint8_t* data )
{
   ( void )ctx;
   printf( "pkt (%zd bytes) -", data_length );
   for ( size_t i = 0; i < data_length; i++ )
   {
      printf( " %02x", data[ i ] );
   }
   printf( "\n" );
}

int main()
{
   // simple packet with byte-stuffed STX
   const uint8_t pkt1[] = { 0x02, 0xFF, 0x10, 0x22, 0x45, 0x03 };
   // packet split across bytestreams
   const uint8_t split_pkt1[] = { 0x02, 0x01, 0x04 };
   const uint8_t split_pkt2[] = { 0x05, 0x06, 0x03 };

   // create a packet decoder
   pkt_decoder_t* decoder = pkt_decoder_create( pkt_printer, nullptr );

   // process the simple packet
   pkt_decoder_write_bytes( decoder, sizeof( pkt1 ), pkt1 );

   // process the split packet
   pkt_decoder_write_bytes( decoder, sizeof( split_pkt1 ), split_pkt1 );
   pkt_decoder_write_bytes( decoder, sizeof( split_pkt2 ), split_pkt2 );

   // clean up the decoder
   pkt_decoder_destroy( decoder );
   return 0;
}
