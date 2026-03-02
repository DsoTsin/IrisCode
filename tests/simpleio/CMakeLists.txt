add_executable(simpleio test_iostream.cc)
target_include_directories(simpleio PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/\$(IntDir)" ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(simpleio PROPERTIES 
    FOLDER "tests"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin")