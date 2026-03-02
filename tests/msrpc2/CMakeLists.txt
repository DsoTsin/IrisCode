set(RPC_PROTOS
RPCServer.idl
RPCServer.Acf)
source_group(proto FILES ${RPC_PROTOS})

add_executable(rpc_client ${RPC_PROTOS} rpc_client.cpp)
target_include_directories(rpc_client PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/\$(IntDir)" ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(rpc_client PROPERTIES 
    FOLDER "tests"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin")

add_executable(rpc_server ${RPC_PROTOS} rpc_server.cpp)
target_include_directories(rpc_server PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/\$(IntDir)" ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(rpc_server PROPERTIES 
    FOLDER "tests"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin")