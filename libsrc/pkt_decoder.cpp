#include "pkt_decoder.h"

#include <iostream>

PacketDecoder::PacketDecoder( pkt_read_fn_t readCallback, void* callbackCtx )
   : m_pktByteList( nullptr ), m_readCallback( readCallback ), m_callbackCtx( callbackCtx )
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
