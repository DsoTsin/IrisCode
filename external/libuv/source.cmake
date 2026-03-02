get_filename_component(libuv_prefix "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

set(_libuv_SOURCES
  src/fs-poll.c
  src/idna.c
  src/inet.c
  src/random.c
  src/strscpy.c
  src/strtok.c
  src/threadpool.c
  src/timer.c
  src/uv-common.c
  src/uv-data-getter-setters.c
)

foreach(_src ${_libuv_SOURCES})
    list(APPEND libuv_SOURCES "${libuv_prefix}/${_src}")
endforeach()

set(_libuv_WIN32_SOURCES
    src/win/async.c
    src/win/core.c
    src/win/detect-wakeup.c
    src/win/dl.c
    src/win/error.c
    src/win/fs.c
    src/win/fs-event.c
    src/win/handle.c
    src/win/getaddrinfo.c
    src/win/loop-watcher.c
    src/win/pipe.c
    src/win/poll.c
    src/win/process.c
    src/win/process-stdio.c
    src/win/signal.c
    src/win/stream.c
    src/win/tcp.c
    src/win/thread.c
    src/win/tty.c
    src/win/udp.c
    src/win/util.c
    src/win/winapi.c
    src/win/winsock.c
)

if (WIN32)
    foreach(_src ${_libuv_WIN32_SOURCES})
        list(APPEND libuv_SOURCES "${libuv_prefix}/${_src}")
    endforeach()
endif (WIN32)

set(libuv_INCLUDE_DIR "${libuv_prefix}/include")
set(libuv_PRIV_INCLUDE_DIR "${libuv_prefix}/src")

source_group("external\\libuv" FILES ${libuv_SOURCES})