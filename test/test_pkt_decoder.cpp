#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "pkt_decoder.h"

TEST_CASE("Create a valid packet decoder", "[pktDecoder]") {
  pkt_decoder_t *decoder = pkt_decoder_create(nullptr, nullptr);
  REQUIRE(decoder != nullptr);
}
