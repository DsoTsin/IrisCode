function(yc_add_lib lib_name)
    cmake_parse_arguments(
        PARSE_ARGV 1 YC # prefix of output variables
        "SHARED" # list of names of the boolean arguments (only defined ones will be true)
        "" # list of names of mono-valued arguments
        "INCS;DEFS;SOURCES;LIBS" # list of names of multi-valued arguments (output variables are lists)
        #${ARGN} # arguments of the function to parse, here we take the all original ones
    )
    # note: if it remains unparsed arguments, here, they can be found in variable PARSED_ARGS_UNPARSED_ARGUMENTS
    #if(NOT PARSED_ARGS_NAME)
    #    message(FATAL_ERROR "You must provide a name")
    #endif(NOT PARSED_ARGS_NAME)
    message("Provided sources are:")
    foreach(src ${YC_SOURCES})
        message("- ${src}")
    endforeach(src)

    foreach(src ${YC_LIBS})
        message("- ${src}")
    endforeach(src)

    if (YC_SHARED)
        add_library(${lib_name} SHARED ${YC_SOURCES})
        target_link_libraries(${lib_name} PRIVATE ${YC_LIBS})
    else()
        add_library(${lib_name} STATIC ${YC_SOURCES})
        target_link_libraries(${lib_name} PRIVATE ${YC_LIBS})
    endif()

    target_compile_definitions(${lib_name} PRIVATE ${YC_DEFS})
    target_include_directories(${lib_name} PRIVATE ${YC_INCS})
    target_include_directories(${lib_name} PRIVATE ${CMAKE_SOURCE_DIR}/src/common)
    
    set_target_properties(${lib_name} PROPERTIES
        FOLDER "ycode/libs"
        CXX_STANDARD 20
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
    )

    add_custom_command(
        TARGET ${lib_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${lib_name}>/ycode/"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${lib_name}>" "$<TARGET_FILE_DIR:${lib_name}>/ycode/"
        COMMAND ${CMAKE_COMMAND} -E remove "$<TARGET_FILE:${lib_name}>"
    )
endfunction()

function(yc_add_bin bin_name)
    cmake_parse_arguments(
        PARSE_ARGV 1 YC # prefix of output variables
        "" # list of names of the boolean arguments (only defined ones will be true)
        "" # list of names of mono-valued arguments
        "SOURCES;LIBS;INCS" # list of names of multi-valued arguments (output variables are lists)
        #${ARGN} # arguments of the function to parse, here we take the all original ones
    )
    # note: if it remains unparsed arguments, here, they can be found in variable PARSED_ARGS_UNPARSED_ARGUMENTS
#    if(NOT PARSED_ARGS_NAME)
#        message(FATAL_ERROR "You must provide a name")
#    endif(NOT PARSED_ARGS_NAME)
    add_executable(${bin_name} ${YC_SOURCES})
    target_link_libraries(${bin_name} PRIVATE ${YC_LIBS})
    target_include_directories(${bin_name} PRIVATE ${YC_INCS})
    
    set_target_properties(${bin_name}
    PROPERTIES
        FOLDER "ycode"
        CXX_STANDARD 20
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
    )
    add_custom_command(
        TARGET ${bin_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${bin_name}>/ycode/"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${bin_name}>" "$<TARGET_FILE_DIR:${bin_name}>/ycode/"
        COMMAND ${CMAKE_COMMAND} -E remove "$<TARGET_FILE:${bin_name}>"
    )
endfunction()