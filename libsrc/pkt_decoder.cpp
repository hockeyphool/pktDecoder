#include "pkt_decoder.h"

#include <cstring>
#include <iostream>
#include <sstream>

PacketDecoder::PacketDecoder( pkt_read_fn_t readCallback, void* callbackCtx )
   : m_pktByteList( nullptr ),
     m_readCallback( readCallback ),
     m_callbackCtx( callbackCtx ),
     m_pktValid( false ),
     m_pktByteListIdx( 0 )
{
}

pkt_decoder_t* pkt_decoder_create( pkt_read_fn_t callback, void* callback_ctx )
{
   auto* decoder = new PacketDecoder( callback, callback_ctx );
   decoder->m_pktByteList = new uint8_t[0];
   return decoder;
}

void pkt_decoder_destroy( pkt_decoder_t* decoder )
{
   decoder->m_readCallback = nullptr;
   decoder->m_callbackCtx = nullptr;
   delete decoder->m_pktByteList;
   delete decoder;
}

void pkt_decoder_write_bytes( pkt_decoder_t* decoder, size_t length, const uint8_t* data )
{
   std::ostringstream oss;
   bool deStuffNextByte( false );

   // Reset the packet byte buffer index if we don't have a valid packet in progress (allows the
   // writer to handle packets that span calls)
   if ( !decoder->m_pktValid )
   {
      decoder->m_pktByteListIdx = 0;
   }

   for ( size_t idx = 0; idx < length; ++idx )
   {
      switch ( data[ idx ] )
      {
         case STX: {
            if ( decoder->m_pktValid )
            {
               oss.str( "" );
               oss << "WARNING: found STX with packet in progress; deleting previous packet data";
               std::cerr << oss.str() << std::endl;
            }
            memset( decoder->m_pktByteList, 0, decoder->m_pktByteListIdx );
            decoder->m_pktByteListIdx = 0;
            decoder->m_pktValid = true;
         }
         break;
         case ETX: {
            decoder->m_pktValid = false;
            if ( decoder->m_readCallback )
            {
               decoder->m_readCallback( decoder->m_callbackCtx,
                                        sizeof( decoder->m_pktByteList ),
                                        decoder->m_pktByteList );
            }
         }
         break;
         case DLE: {
            deStuffNextByte = true;
         }
         break;
         default: {
            // handle a non-control byte (only if currently processing a packet)
            if ( decoder->m_pktValid )
            {
               uint8_t currentByte = data[ idx ];
               if ( deStuffNextByte )
               {
                  currentByte |= ENC;
                  deStuffNextByte = false;
               }
               decoder->m_pktByteList[ decoder->m_pktByteListIdx++ ] = currentByte;
            }
         }
      }
   }
}
