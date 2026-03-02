cmake_minimum_required(VERSION 3.16)

set(VCPKG_TARGET_TRIPLET "x86-windows" CACHE STRING "")
project(DeveloperService LANGUAGES C CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS on)

include (cmake/modules.cmake)

include_directories(
    include
    ${CMAKE_SOURCE_DIR}/external/thrift/include
    ${CMAKE_SOURCE_DIR}/external/libplist/include
    ${CMAKE_SOURCE_DIR}/external/libplist/libcnary/include
    )
    
if(WIN32)
    set(THRIFT_COMPILER ${CMAKE_SOURCE_DIR}/external/thrift/bin/thrift.exe)
else()
    set(THRIFT_COMPILER ${CMAKE_SOURCE_DIR}/external/thrift/bin/thrift)
endif()

set(THRIFT_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/thrift/gen/)
file(MAKE_DIRECTORY ${THRIFT_OUT_DIR})

set(THRIFT_GEN_SRCS
    ${THRIFT_OUT_DIR}/developer.cpp
    ${THRIFT_OUT_DIR}/developer_types.cpp
    ${THRIFT_OUT_DIR}/developer_constants.cpp
)
set_source_files_properties(${THRIFT_GEN_SRCS} PROPERTIES GENERATED 1)

set(THRIFT_SOURCES
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/TApplicationException.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/TOutput.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/thrift-tl.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/async/TAsyncChannel.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/async/TAsyncProtocolProcessor.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/async/TConcurrentClientSyncInfo.h
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/async/TConcurrentClientSyncInfo.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/concurrency/ThreadManager.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/concurrency/TimerManager.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/processor/PeekProcessor.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/protocol/TBase64Utils.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/protocol/TDebugProtocol.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/protocol/TJSONProtocol.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/protocol/TMultiplexedProtocol.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/protocol/TProtocol.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/TTransportException.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/TFDTransport.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/TSimpleFileTransport.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/THttpTransport.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/THttpClient.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/THttpServer.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/TSocket.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/TSocketPool.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/TServerSocket.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/TTransportUtils.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/TBufferTransports.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/server/TConnectedClient.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/server/TServerFramework.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/server/TSimpleServer.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/server/TThreadPoolServer.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/server/TThreadedServer.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/TPipe.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/TPipeServer.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/transport/TFileTransport.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/concurrency/ThreadFactory.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/concurrency/Thread.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/concurrency/Monitor.cpp
    ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/concurrency/Mutex.cpp
)

if(WIN32)
    list(APPEND THRIFT_SOURCES
        ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/windows/TWinsockSingleton.cpp
        ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/windows/SocketPair.cpp
        ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/windows/GetTimeOfDay.cpp
        ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/windows/WinFcntl.cpp
        ${CMAKE_SOURCE_DIR}/external/thrift/src/thrift/windows/OverlappedSubmissionThread.cpp)
endif(WIN32)

set(PLIST_SOURCES
    external/libplist/src/Array.cpp
    external/libplist/src/base64.c
    external/libplist/src/Boolean.cpp
    external/libplist/src/bplist.c
    external/libplist/src/bytearray.c
    external/libplist/src/Data.cpp
    external/libplist/src/Date.cpp
    external/libplist/src/Dictionary.cpp
    external/libplist/src/hashtable.c
    external/libplist/src/Integer.cpp
    external/libplist/src/Key.cpp
    external/libplist/src/Node.cpp
    external/libplist/src/plist.c
    external/libplist/src/ptrarray.c
    external/libplist/src/Real.cpp
    external/libplist/src/String.cpp
    external/libplist/src/Structure.cpp
    external/libplist/src/time64.c
    external/libplist/src/Uid.cpp
    external/libplist/src/xplist.c
    external/libplist/libcnary/node.c
    external/libplist/libcnary/node_list.c
    )

include_directories(${THRIFT_OUT_DIR})

add_custom_command(OUTPUT generated.timestamp
    COMMAND ${THRIFT_COMPILER} --gen cpp:no_default_operators -out ${THRIFT_OUT_DIR} -r ${CMAKE_SOURCE_DIR}/src/developer.thrift
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating source code from Thrift definition")
add_custom_target(RunThriftCompiler DEPENDS generated.timestamp
    COMMENT "Checking if re-generation is required")
set_target_properties(RunThriftCompiler PROPERTIES FOLDER "service/proto")

set(DEVSVR_SOURCES
    src/developer.thrift
    src/server.cpp
    src/server.h
    src/hooks.cpp
    src/anisette.cpp
    src/anisette.h
    src/authutils.cpp
    src/authutils.h
    src/developer_api.cpp
    src/developer_api.h
    src/gsa_client_verify.cpp
    src/gsa_client_fetch_token.cpp
    src/gsa_client_auth.cpp
    src/gsa_client.cpp
    src/gsa_client.h
    src/service_client_app.cpp
    src/service_client_csr.cpp
    src/service_client_provision.cpp
    src/service_client.cpp
    src/service_client.h
    src/exception.cpp
    src/exception.h
    src/csr.cpp
    src/csr.h
    src/common_priv.h
    )

source_group(Dev FILES ${DEVSVR_SOURCES})
source_group("Dev\\Proto" FILES ${THRIFT_GEN_SRCS})
source_group(ThriftFramework FILES ${THRIFT_SOURCES})
source_group(Plist FILES ${PLIST_SOURCES})

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    add_library(adsapi SHARED
        src/apple_developer_service_api.cpp
        src/exception.cpp
        ${THRIFT_SOURCES}
        ${THRIFT_GEN_SRCS}
        )
    set_target_properties(adsapi PROPERTIES
        CXX_STANDARD 17
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
        )
    target_compile_definitions(adsapi PRIVATE THRIFT_STATIC_DEFINE BUILD_LIB)
    add_dependencies(adsapi RunThriftCompiler)

    add_executable(test_adsapi src/test_adsapi.cpp)
    target_link_libraries(test_adsapi PRIVATE adsapi)

else () 
    # add developer service
    add_executable(ads_daemon 
        ${DEVSVR_SOURCES}
        ${THRIFT_SOURCES}
        ${THRIFT_GEN_SRCS}
        ${PLIST_SOURCES}
        )
    target_link_libraries(ads_daemon PRIVATE 
        ext::corecrypto
        #ext::curl
        ext::zlib 
        ext::ssl
        ext::crypto
        ext::detours
        ext::brotlicommon
        ext::brotlidec
        ext::brotlienc
        ext::cpprestsdk
        crypt32
        winhttp
        bcrypt
        )
    target_precompile_headers(ads_daemon PRIVATE "src/common_priv.h")
    set_target_properties(ads_daemon PROPERTIES
        CXX_STANDARD 17
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
        )
    target_compile_definitions(ads_daemon PRIVATE THRIFT_STATIC_DEFINE)
    add_dependencies(ads_daemon RunThriftCompiler)

    add_library(adsapi32 SHARED
        include/apple_developer_service_api.h
        src/apple_developer_service_api.cpp
        src/exception.cpp
        ${THRIFT_SOURCES}
        ${THRIFT_GEN_SRCS}
        )
    target_compile_definitions(adsapi32 PRIVATE THRIFT_STATIC_DEFINE BUILD_LIB)
    set_target_properties(adsapi32 PROPERTIES
        CXX_STANDARD 17
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
        )
    add_dependencies(adsapi32 RunThriftCompiler)
endif ()