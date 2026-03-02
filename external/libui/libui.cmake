get_filename_component(install_prefix "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

add_library(ext::libui SHARED IMPORTED)

if(WIN32)
    set_property(TARGET ext::libui APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
    set_property(TARGET ext::libui APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

    if( "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64")
        set_target_properties(ext::libui PROPERTIES 
            IMPORTED_LOCATION_DEBUG "${install_prefix}/bin/windows/arm64/libui.dll"
            IMPORTED_IMPLIB_DEBUG   "${install_prefix}/lib/arm64/libui.lib"
            IMPORTED_LOCATION_RELEASE "${install_prefix}/bin/windows/arm64/libui.dll"
            IMPORTED_IMPLIB_RELEASE "${install_prefix}/lib/arm64/libui.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include"
        )
        set(libui_DLL "${install_prefix}/bin/windows/arm64/libui.dll")
    else()
        set_target_properties(ext::libui PROPERTIES 
            IMPORTED_LOCATION_DEBUG "${install_prefix}/bin/windows/x64/libui.dll"
            IMPORTED_IMPLIB_DEBUG "${install_prefix}/lib/x64/libui.lib"
            IMPORTED_LOCATION_RELEASE "${install_prefix}/bin/windows/x64/libui.dll"
            IMPORTED_IMPLIB_RELEASE "${install_prefix}/lib/x64/libui.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include"
        )
        set(libui_DLL "${install_prefix}/bin/windows/x64/libui.dll")
    endif()

elseif(APPLE)
    set_target_properties(ext::libui PROPERTIES
        IMPORTED_LOCATION "${install_prefix}/bin/macOS/libui.A.dylib"
        INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include")
    set(libui_DLL "${install_prefix}/bin/macOS/libui.A.dylib")
    message(STATUS "LibUI link to ${libui_DLL}")
endif()
