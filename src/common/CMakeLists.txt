enable_language(C CXX ASM) 

if ( "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64" )
enable_language(ASM_MARMASM)
endif()

set(COMMON_SRCS 
	hash_map.cc 
	source_file.cc 
	os.cc os.hpp 
	log.hpp log.cc 
	md5.hpp md5.cc
	base64.hpp base64.cc
    zip.hpp zip.cc
    rpc.hpp rpc.cc
    download.hpp download.cc
    decrypt.hpp decrypt.cc
    pe_walker.hpp pe_walker.cc
    machine_state.hpp machine_state.cc
    config.hpp config.cc
    args_parse.hpp args_parse.cc)
if(WIN32)
list(APPEND COMMON_SRCS ${CMAKE_SOURCE_DIR}/external/std.ext/src/win32.cpp
    ntsrv.hpp ntsrv.cc
    ntsocket.h ntsocket.cc
    handle.h handle.cc)
endif()

list(APPEND COMMON_SRCS 
    ${CMAKE_SOURCE_DIR}/external/std.ext/src/socket.cpp
    ${CMAKE_SOURCE_DIR}/external/tinyxml2/tinyxml2.cpp)

source_group(src FILES ${COMMON_SRCS})

set(_ZIP_SRCS
#CDirEntry.cpp
#tools.cpp
zipper.cpp
unzipper.cpp
minizip/zip.c
minizip/unzip.c
minizip/ioapi.c
minizip/mztools.c
)
if(WIN32)
    list(APPEND _ZIP_SRCS "minizip/iowin32.c")
endif()
foreach(_ZIP_SRC ${_ZIP_SRCS})
    list(APPEND ZIP_SRCS "${CMAKE_SOURCE_DIR}/external/zipper/${_ZIP_SRC}")
endforeach()

list (APPEND ZIP_SRCS 
    "${CMAKE_SOURCE_DIR}/external/lzma/meta7z.cpp"    
    "${CMAKE_SOURCE_DIR}/external/lzma/meta7z.h"
)

if (WIN32)
  if ( "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64" )
    list(APPEND ZIP_SRCS "${CMAKE_SOURCE_DIR}/external/lzma/Asm/arm64/7zAsm.S")
  else ()
    list(APPEND ZIP_SRCS "${CMAKE_SOURCE_DIR}/external/lzma/Asm/x86/AesOpt.asm")
  endif ()
endif()

if(APPLE)
  execute_process(
      COMMAND uname -m
      RESULT_VARIABLE result
      OUTPUT_VARIABLE APPLE_HOST_ARCH
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(${APPLE_HOST_ARCH} STREQUAL "arm64")
    message(STATUS "Apple Silicon")
    #list(APPEND ZIP_SRCS "${CMAKE_SOURCE_DIR}/external/lzma/Asm/x86/AesOpt.asm")
  endif()
endif()

source_group("external\\zipper" FILES ${ZIP_SRCS})

set(_JSON_CPP_SRCS
${CMAKE_SOURCE_DIR}/external/jsoncpp/src/json_value.cpp
${CMAKE_SOURCE_DIR}/external/jsoncpp/src/json_reader.cpp
${CMAKE_SOURCE_DIR}/external/jsoncpp/src/json_writer.cpp)
source_group("external\\jsoncpp" FILES ${_JSON_CPP_SRCS})

source_group("external\\mimalloc" FILES ${mimalloc_SOURCES})

add_library(common STATIC ${COMMON_SRCS} ${ZIP_SRCS} ${_JSON_CPP_SRCS} ${mimalloc_SOURCES})
target_include_directories(common PUBLIC 
    "${CMAKE_SOURCE_DIR}/external/jsoncpp/include"
    "${CMAKE_SOURCE_DIR}/external/lzma"
    ${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(common PROPERTIES 
    FOLDER "irisbuild"
    CXX_STANDARD 20
    OUTPUT_NAME "common-static"
    MSVC_RUNTIME_LIBRARY "MultiThreaded")

if(ENABLE_SQLITE)
    set(_SQLITE_SRCS
    ${CMAKE_SOURCE_DIR}/external/sqlite/sqlite3.h
    ${CMAKE_SOURCE_DIR}/external/sqlite/sqlite3.c
    ${CMAKE_SOURCE_DIR}/external/sqlite/sqlite3ext.h)
    source_group("external\\sqlite3" FILES ${_SQLITE_SRCS})
    target_include_directories(common PUBLIC
        ${CMAKE_SOURCE_DIR}/external/sqlite
        ${CMAKE_SOURCE_DIR}/external/lzma
    )
    target_sources(common PRIVATE ${_SQLITE_SRCS})
endif()

if(NOT WIN32)
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        add_definitions(-DHAS_OPENSSL=1)
        target_include_directories(common PRIVATE ${OPENSSL_INCLUDE_DIR})
        target_link_libraries(common ${OPENSSL_LIBRARIES})
    endif()
endif()

if(WIN32)
    target_link_libraries(common skia::skia skia::spirvcross crypt32 winhttp shlwapi userenv user32 shell32 advapi32)
endif()

target_include_directories(common PRIVATE 
${CMAKE_SOURCE_DIR}/external/zipper
${CMAKE_SOURCE_DIR}/external/zipper/minizip)

if(APPLE)
    target_link_libraries(common z curl "-framework CoreFoundation")
else()
    target_link_libraries(common ZLIB::ZLIB)
endif()

target_compile_definitions(common PRIVATE HAVE_CONFIG_H=1 _CRT_SECURE_NO_WARNINGS=1 SK_DIRECT3D SK_GANESH)

if(WIN32)
    #target_link_libraries(common ext::detours)
    source_group("external\\detours" FILES ${_DETOURS_SRCS})
    target_include_directories(common PUBLIC ${_DETOURS_INC})
    target_sources(common PRIVATE ${_DETOURS_SRCS})
endif()

add_library(libuv ${libuv_SOURCES})
target_include_directories(libuv PUBLIC ${libuv_INCLUDE_DIR})
target_include_directories(libuv PRIVATE ${libuv_PRIV_INCLUDE_DIR})
set_target_properties(libuv
PROPERTIES 
    FOLDER "irisbuild" 
    MSVC_RUNTIME_LIBRARY "MultiThreaded"
)
target_link_libraries(libuv Ws2_32 Iphlpapi Psapi)