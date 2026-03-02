get_filename_component(install_prefix "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

add_library(ext::detours SHARED IMPORTED)

set(_DETOURS_SRCS
${install_prefix}/source/detours.cpp
${install_prefix}/source/creatwth.cpp
${install_prefix}/source/disolx64.cpp
${install_prefix}/source/image.cpp
${install_prefix}/source/modules.cpp
${install_prefix}/source/disasm.cpp
#${install_prefix}/source/uimports.cpp
)
set(_DETOURS_INC "${install_prefix}/include")

if(WIN32)
    if( "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64")
       set_target_properties(ext::detours PROPERTIES
            IMPORTED_IMPLIB "${install_prefix}/lib.vc141.ARM64/detours.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include"
        )
    else()
        set_target_properties(ext::detours PROPERTIES
            IMPORTED_IMPLIB "${install_prefix}/lib.vc141.x64/detours.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include"
        )
    endif()
elseif(APPLE)
    set_target_properties(ext::detours PROPERTIES
        IMPORTED_LOCATION "${install_prefix}/bin/libzlib.A.dylib"
        INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include")
endif()