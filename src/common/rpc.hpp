#pragma once

#include "stl.hpp"

namespace iris {
// protocol
namespace proto {
enum class value_type : uint8_t { i8, u8, i16, u16, i32, u32, i64, u64, buf, ptr };

class serializer {
public:
  serializer& ser(int8_t& val);
  serializer& ser(int16_t& val);
  serializer& ser(int32_t& val);
  serializer& ser(int64_t& val);

  serializer& ser(uint8_t& val);
  serializer& ser(uint16_t& val);
  serializer& ser(uint32_t& val);
  serializer& ser(uint64_t& val);

  serializer& ser(void* buf, uint64_t& len);

private:
};
} // namespace proto
// transport
namespace trans {
class stream {
public:
  virtual ~stream() {}
  virtual bool bytes_ready(uint64_t& length) const = 0;
  virtual uint64_t read() = 0;
  virtual uint64_t write() = 0;
};
} // namespace trans

} // namespace iris