get_filename_component(install_prefix "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

add_library(ZLIB::ZLIB SHARED IMPORTED)

set(ZLIB_INCLUDE_DIR "${install_prefix}/include")

if(WIN32)
    if( "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64" )
        set_target_properties(ZLIB::ZLIB PROPERTIES 
            IMPORTED_IMPLIB "${install_prefix}/lib/arm64_VS2017/zlib.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include"
        )
    else()
        set_target_properties(ZLIB::ZLIB PROPERTIES
            IMPORTED_IMPLIB "${install_prefix}/lib/x64_VS2017/zlib.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include"
        )
    endif()

    #arm64_VS2017
    set(ZLIB_LIBRARY "${install_prefix}/lib/x64_VS2017/zlib.lib")
elseif(APPLE)
    set_target_properties(ZLIB::ZLIB PROPERTIES
        IMPORTED_LOCATION "${install_prefix}/bin/libzlib.A.dylib"
        INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include")
    set(ZLIB_LIBRARY "${install_prefix}/bin/libzlib.A.dylib")
endif()