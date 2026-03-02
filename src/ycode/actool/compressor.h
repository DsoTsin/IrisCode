#pragma once
#include "ycore_config.h"

#include <stdint.h>

enum class CompressAlgorithm {
  ZLIB,
  LZ4,
  LZFSE,
  RLE
};

#if __cplusplus
extern "C" {
#endif

YCORE_API size_t compression_encode_buffer(uint8_t* destBuffer,
                                           size_t destSize,
                                           const uint8_t* srcBuffer,
                                           size_t srcSize,
                                           CompressAlgorithm algorithm);
YCORE_API size_t compression_decode_buffer(uint8_t* destBuffer,
                                           size_t destSize,
                                           const uint8_t* srcBuffer,
                                           size_t srcSize,
                                           CompressAlgorithm algorithm);

#if __cplusplus
}
#endif