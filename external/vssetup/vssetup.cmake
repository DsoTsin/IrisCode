get_filename_component(install_prefix "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

if(WIN32)
    add_library(ext::vssetup SHARED IMPORTED)

    set_property(TARGET ext::vssetup APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
    set_property(TARGET ext::vssetup APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

    if( "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64")
        set_target_properties(ext::vssetup PROPERTIES 
            IMPORTED_IMPLIB "${install_prefix}/v141/arm64/Microsoft.VisualStudio.Setup.Configuration.Native.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include"
        )
    else()
        set_target_properties(ext::vssetup PROPERTIES 
            IMPORTED_IMPLIB "${install_prefix}/v141/x64/Microsoft.VisualStudio.Setup.Configuration.Native.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include"
        )
    endif()
endif()
