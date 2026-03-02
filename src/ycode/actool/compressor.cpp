#include "compressor.h"

#include "lz4/lz4.h"
#include "lzfse.h"
#include "zlib.h"

extern "C" size_t lzvn_encode_buffer(void* dst,
                                     size_t dst_size,
                                     const void* src,
                                     size_t src_size,
                                     void* work);

size_t compression_encode_buffer(uint8_t* destBuffer,
                                 size_t destSize,
                                 const uint8_t* srcBuffer,
                                 size_t srcSize,
                                 CompressAlgorithm algorithm) {
  switch (algorithm) {
  case CompressAlgorithm::LZ4:
    return LZ4_compress_default((const char*)srcBuffer, (char*)destBuffer, srcSize, destSize);
  case CompressAlgorithm::LZFSE:
    return lzfse_encode_buffer(destBuffer, destSize, srcBuffer, srcSize, NULL);
  case CompressAlgorithm::ZLIB: {
    uLongf destSz = destSize;
    auto ret = compress2(destBuffer, &destSz, srcBuffer, srcSize, 5);
    return ret == Z_OK ? destSz : 0;
  }
  default:
    return 0;
  }
}

size_t compression_decode_buffer(uint8_t* destBuffer,
                                 size_t destSize,
                                 const uint8_t* srcBuffer,
                                 size_t srcSize,
                                 CompressAlgorithm algorithm) {
  switch (algorithm) {
  case CompressAlgorithm::LZ4:
    return LZ4_decompress_safe((const char*)srcBuffer, (char*)destBuffer, srcSize, destSize);
  case CompressAlgorithm::LZFSE:
    return lzfse_decode_buffer(destBuffer, destSize, srcBuffer, srcSize, NULL);
  case CompressAlgorithm::ZLIB: {
    uLongf destSz = destSize;
    auto ret = uncompress(destBuffer, &destSz, srcBuffer, srcSize);
    return ret == Z_OK ? destSz : 0;
  }
  default:
    return 0;
  }
}

#include "lz4/lz4.c"
#include "lz4/lz4file.c"
#include "lz4/lz4frame.c"
#include "lz4/lz4hc.c"
#include "lz4/xxhash.c"