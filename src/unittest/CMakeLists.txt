include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../parser)

add_ib_test(rpc_test_server SOURCES rpc_server.cc LIBRARIES common libuv)
add_ib_test(rpc_test_client SOURCES rpc_client.cc LIBRARIES common)
add_ib_test(machine_state SOURCES machine_state.cc LIBRARIES common)
add_ib_test(gui_test SOURCES gui_test.cc LIBRARIES common core.ui)
add_ib_test(code_editor_test SOURCES code_editor_test.cc LIBRARIES common core.ui)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
if(WIN32)
  add_ib_test(handle_test SOURCES handle_test.cc LIBRARIES common)
endif(WIN32)
