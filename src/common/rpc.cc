#include "rpc.hpp"

//#include <Windows.h>

namespace iris {
namespace proto {
serializer& serializer::ser(int8_t& val) {
  return *this;
}
serializer& serializer::ser(int16_t& val) {
  return *this;
}
serializer& serializer::ser(int32_t& val) {
  return *this;
}
serializer& serializer::ser(int64_t& val) {
  return *this;
}
serializer& serializer::ser(uint8_t& val) {
  return *this;
}
serializer& serializer::ser(uint16_t& val) {
  return *this;
}
serializer& serializer::ser(uint32_t& val) {
  return *this;
}
serializer& serializer::ser(uint64_t& val) {
  return *this;
}
serializer& serializer::ser(void* buf, uint64_t& len) {
  return *this;
}
} // namespace proto

namespace trans {
enum flags {
  flag_reply = 1 << 0,
  flag_async = 1 << 1,
  flag_exception = 1 << 2,
  flag_compressed = 1 << 3,
};

struct message_hdr {
  uint32_t magic;
  uint32_t seqid;
  uint8_t flags;          // compressed | has exception | no_return | async | is reply ?
  uint8_t this_length;    // Fixed 16 bytes
  uint8_t fragment_id;    // usually 0
  uint8_t fragment_count; // usually 1
  uint32_t length;        // payload length
};
static_assert(sizeof(message_hdr) == 16, "invalid packet header size!");

// complete message is | message_hdr | message_body |
// message body | method_name [u8 (str_len) : ...] | (method_arguments | (response + exception) ) |

void test() {
  //BOOL WINAPI ReadFile(_In_ HANDLE hFile,
  //                     _Out_writes_bytes_to_opt_(nNumberOfBytesToRead, *lpNumberOfBytesRead)
  //                         __out_data_source(FILE) LPVOID lpBuffer,
  //                     _In_ DWORD nNumberOfBytesToRead, _Out_opt_ LPDWORD lpNumberOfBytesRead,
  //                     _Inout_opt_ LPOVERLAPPED lpOverlapped);
}

//void MyReadFile(HANDLE hFile,
//                LPVOID lpBuffer,
//                DWORD nNumberOfBytesToRead,
//                LPDWORD lpNumberOfBytesRead) {

  //client c;
  //c.begin_method("ReadFile");
  //c.write(hFile);
  //c.write(nNumberOfBytesToRead);
  //c.read(lpNumberOfBytesRead);
  //c.read(lpBuffer, lpNumberOfBytesRead);
  //c.end_method();
  //// service side
  //conn.get_method();
  //conn.read(hFile);
  //conn.read(nNumberOfBytesToRead);
  //// return bytes
  //DWORD ret = 0;
  //// allocate buffer
  //BOOL r = ::ReadFile(hFile, buffer, nNumberOfBytesToRead, &ret, NULL);
  //conn.write(ret);
  //conn.write(buffer);
  //conn.write(r);
//}

} // namespace trans

} // namespace iris
