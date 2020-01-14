#include "pkt_decoder.h"

#include <cstring>

PacketDecoder::PacketDecoder( pkt_read_fn_t readCallback, void* callbackCtx )
   : m_packetBuffer( nullptr ),
     m_pktBufIdx( 0 ),
     m_pktValid( false ),
     m_readCallback( readCallback ),
     m_callbackCtx( callbackCtx ),
     m_deStuffNextByte( false )
{
}

pkt_decoder_t* pkt_decoder_create( pkt_read_fn_t callback, void* callback_ctx )
{
   auto* decoder = new PacketDecoder( callback, callback_ctx );
   decoder->m_packetBuffer = new uint8_t[ MAX_DECODED_DATA_LENGTH ];
   decoder->clearBuffer();
   return decoder;
}

void pkt_decoder_destroy( pkt_decoder_t* decoder )
{
   decoder->m_readCallback = nullptr;
   decoder->m_callbackCtx = nullptr;
   delete decoder->m_packetBuffer;
   delete decoder;
}

void pkt_decoder_write_bytes( pkt_decoder_t* decoder, size_t length, const uint8_t* data )
{
   for ( size_t idx = 0; idx < length; ++idx )
   {
      switch ( data[ idx ] )
      {
         case STX: {
            // If we already have a packet in progress this will cause it to be silently dropped
            decoder->clearBuffer();
            decoder->m_pktValid = true;
         }
         break;
         case ETX: {
            // do NOT call the callback function if a) we're not working on a valid packet, b) we
            // haven't inserted any decoded bytes into the packet buffer, or c) we don't have a
            // valid callback pointer
            if ( decoder->m_pktValid && ( decoder->m_pktBufIdx > 0 ) && decoder->m_readCallback )
            {
               decoder->m_readCallback(
                  decoder->m_callbackCtx, decoder->m_pktBufIdx, decoder->m_packetBuffer );
            }
            decoder->m_pktValid = false;
         }
         break;
         case DLE: {
            decoder->m_deStuffNextByte = true;
         }
         break;
         default: {
            // handle a non-control byte (only if currently processing a packet)
            if ( decoder->m_pktValid )
            {
               uint8_t currentByte = data[ idx ];
               if ( decoder->m_deStuffNextByte )
               {
                  currentByte &= ~ENC;
                  decoder->m_deStuffNextByte = false;
               }
               if ( MAX_DECODED_DATA_LENGTH > decoder->m_pktBufIdx )
               {
                  decoder->m_packetBuffer[ decoder->m_pktBufIdx++ ] = currentByte;
               }
               else
               {
                  // Silently fail because this byte will exceed the allowed maximum packet
                  // length, and prevent handling of any further bytes until a new STX is received
                  decoder->m_pktValid = false;
               }
            }
         }
      }
   }
}

void PacketDecoder::clearBuffer()
{
   memset( this->m_packetBuffer, 0, MAX_DECODED_DATA_LENGTH );
   this->m_pktBufIdx = 0;
}
