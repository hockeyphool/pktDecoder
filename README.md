# pktDecoder
A simple DLE/STX/ETX packet framer/decoder library

## Description
This project implements the `libpktdecoder` static library to process byte streams and find packets framed with **Start of Text** (STX - 0x02) and **End of Text** (ETX - 0x03) markers. If the bytestream needs to include a control character it will be preceded by a **Data Link Escape** (DLE - 0x10) marker, and will be byte-stuffed by *OR*-ing it with an encoding character (0x20).

## Library Contents
### Library Definitions
`typedef struct PacketDecoder pkt_decoder_t`
A PacketDecoder object

`typedef void ( *pkt_read_fn_t )( void* ctx, size_t data_length, const uint8_t* data )`
Signature for the user-provided callback function

### Library Methods
The packet decoder library provides the following methods:

`pkt_decoder_t* pkt_decoder_create( pkt_read_fn_t callback, void* callback_ctx )`
Creates and initializes an instance of a PacketDecoder and returns a pointer to the object. The user is responsible for providing the pointers to the callback function and callback context; either can be a *nullptr*.

`void pkt_decoder_destroy( pkt_decoder_t* decoder )`
Deletes a PacketDecoder object and associated memory.

`void pkt_decoder_write_bytes( pkt_decoder_t* decoder, size_t len, const uint8_t* data )`
Processes the passed-in byte stream. This method looks for **STX**, **DLE**, and **ETX** markers and handles them as follows:
- **STX** -- Starts a new packet. If a packet was already in progress it will be silently dropped.
- **DLE** -- Recognizes that the next byte needs to be unstuffed before being added to the packet. This allows the byte stream to include **STX**, **DLE**, and **ETX** characters
- **ETX** -- Completes processing of a packet, and calls the callback function (if provided). 

**NOTE** The callback will not be called if the current packet buffer is empty (e.g. the packet decoder received a byte stream that did not include a **STX** control character)
Bytes other than these control characters will be added to the in-progress packet buffer once **STX** has been seen.

**NOTE** The library will allow a maxiumum of 512 bytes to be processed. If the library is given data that exceeds this limit (post-processing) it will silently discard the in-progress packet buffer and stop processing any new bytes until another **STX** character is received.

## Usage
To make use of the `libpktdecoder` library:
1. Include `pkt_decoder.h` in your source
1. Link your source to `libpktdecoder.a`
1. Define a callback method to receive the processed packets
1. Create a packet decoder instance, and use the `pkt_write_bytes()` method to pass it streams of bytes to process
- Ensure your byte stream starts with a **STX** character. (Any preceding characters will be ignored)
- If you need to include **STX**, **ETX**, or **DLE** characters in the packet data, do the following:
1. Insert a **DLE** character in the byte stream
1. Byte-stuff the intended character by performing a bitwise **OR** with the value `0x020`
`uint8_t new_char( STX | 0x20);`
1. Insert the byte-stuffed character in the stream after the **DLE** 
- Ensure your byte stream ends with a **ETX** character. (Any trailing characters will be ignored)

Take a look at `example.cpp` in the `src` folder to see how the use the library methods
