#ifndef PKT_DECODER_H_INCLUDED
#define PKT_DECODER_H_INCLUDED

#include <cstdint>
#include <cstdlib>

#ifdef __cplusplus
extern "C"
{
#endif
#define MAX_DECODED_DATA_LENGTH ( 512 )

   const uint8_t STX = 0x02;
   const uint8_t ETX = 0x03;
   const uint8_t DLE = 0x10;
   const uint8_t ENC = 0x20;

   class PacketDecoder;

   typedef struct PacketDecoder pkt_decoder_t;
   // data_length must be <= MAX_DECODED_DATA_LENGTH
   typedef void ( *pkt_read_fn_t )( void* ctx, size_t data_length, const uint8_t* data );
   // Constructor for a pkt_decoder
   pkt_decoder_t* pkt_decoder_create( pkt_read_fn_t callback, void* callback_ctx );
   // Destructor for a pkt_decoder
   void pkt_decoder_destroy( pkt_decoder_t* decoder );
   // Called on incoming, undecoded bytes to be translated into packets
   void pkt_decoder_write_bytes( pkt_decoder_t* decoder, size_t len, const uint8_t* data );

   class PacketDecoder
   {
    public:
      PacketDecoder( pkt_read_fn_t, void* );
      virtual ~PacketDecoder() = default;
      void clearBuffer();

      uint8_t* m_packetBuffer;
      size_t m_pktBufIdx;
      bool m_pktValid;
      pkt_read_fn_t m_readCallback;
      void* m_callbackCtx;
      bool m_deStuffNextByte;
   };

#ifdef __cplusplus
}
#endif
#endif // PKT_DECODER_H_INCLUDED
