#ifndef __NT_SOCKET__
#define __NT_SOCKET__
#pragma once

#include <stdint.h>

// Client implementation
#if __cplusplus
extern "C" {
#endif
void __nt_socket_init();
uintptr_t __nt_socket_create(int family, int type, int proto);
int __nt_socket_close(uintptr_t socket);

int __nt_socket_bind(uintptr_t socket, const void* address, int share_type);
int __nt_socket_connect(uintptr_t socket, const void* address);
int __nt_socket_recv(uintptr_t socket, void* buffer, uint32_t* bytes_read);
int __nt_socket_send(uintptr_t socket, const void* buffer, uint32_t length, int flags);
// int __nt_socket_listen(uintptr_t socket, int backlog);
// int __nt_socket_accept(uintptr_t socket, const void* address, int family, int sock_type, int
// proto);
int __nt_socket_shutdown(uintptr_t socket, int how);
#if __cplusplus
}
#endif

#if __cplusplus
#include "stl.hpp"
namespace iris {
namespace nt {
class socket {
public:
  socket();
  virtual ~socket();

  bool connect(const char* ipv4, int port);
  bool send(const string& data);
  bool recv(void* data, uint32_t& length);
  bool shutdown(int how);

private:
  std::mutex mutex;
  uintptr_t _;
};
} // namespace nt
} // namespace iris
#endif
#endif