get_filename_component(install_prefix "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

add_library(rust::sevenz SHARED IMPORTED)

set_property(TARGET rust::sevenz APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_property(TARGET rust::sevenz APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
if(WIN32)
    if( "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64")
        set_target_properties(rust::sevenz PROPERTIES 
            IMPORTED_IMPLIB "${install_prefix}/lib/winarm64/sevenz_rust.lib"
        )
    else()
        set_target_properties(rust::sevenz PROPERTIES
            IMPORTED_IMPLIB "${install_prefix}/lib/win64/sevenz_rust.lib"
        )
    endif()
elseif(APPLE)
    set_target_properties(ZLIB::ZLIB PROPERTIES
        IMPORTED_LOCATION "${install_prefix}/bin/libzlib.A.dylib"
        INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include")
    set(ZLIB_LIBRARY "${install_prefix}/bin/libzlib.A.dylib")
endif()