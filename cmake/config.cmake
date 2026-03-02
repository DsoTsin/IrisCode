set_property(GLOBAL PROPERTY USE_FOLDERS on)
include(FetchContent)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/pdb)

set(CMAKE_NDARD 17)

if(MSVC)
	add_definitions(/MP)
endif()

set(COPY_RIGHT "Tsinworks (C) 2011-2025")

function(generate_win_res TARGET)
    cmake_parse_arguments(${TARGET}
        ""
        "VER;DESC;ICON;ICON2"
        ""
        ${ARGN}
    )
    set(__TEMPLATE_RC "\
    \#include <windows.h>\r\n\
    VS_VERSION_INFO VERSIONINFO\r\n\
        FILEVERSION 1,2,2,0\r\n\
        PRODUCTVERSION 1,2,2,0\r\n\
        FILEFLAGSMASK 0x3fL\r\n\
    \#ifdef _DEBUG\r\n\
        FILEFLAGS VS_FF_DEBUG\r\n\
    \#else\r\n\
        FILEFLAGS 0x0L\r\n\
    \#endif\r\n\
        FILEOS VOS__WINDOWS32\r\n\
        FILETYPE VFT_DLL\r\n\
        FILESUBTYPE 0x0L\r\n\
        BEGIN\r\n\
            BLOCK \"StringFileInfo\"\r\n\
            BEGIN\r\n\
                BLOCK \"040904b0\"\r\n\
                BEGIN\r\n\
                    VALUE \"CompanyName\", \"{COMPANY}\\0\"\r\n\
                    VALUE \"FileDescription\", \"{DESCRIPTION}\\0\"\r\n\
                    VALUE \"FileVersion\", \"{_VERSION_}\\0\"\r\n\
                    VALUE \"LegalCopyright\", \"{COPYRIGHT}\\0\"\r\n\
                    VALUE \"ProductName\", \"{PRODUCT}\\0\"\r\n\
                    VALUE \"ProductVersion\", \"{_VERSION_}\\0\"\r\n\
                END\r\n\
            END\r\n\
            BLOCK \"VarFileInfo\"\r\n\
            BEGIN\r\n\
                VALUE \"Translation\", 0x0409, 1200\r\n\
            END\r\n\
        END\r\n\
    ")
    string(REPLACE "{_VERSION_}" "${${TARGET}_VER}" __TEMPLATE_RC ${__TEMPLATE_RC})
    string(REPLACE "{DESCRIPTION}" "${${TARGET}_DESC}" __TEMPLATE_RC ${__TEMPLATE_RC})
    string(REPLACE "{COPYRIGHT}" ${COPY_RIGHT} __TEMPLATE_RC ${__TEMPLATE_RC})
    string(REPLACE "{PRODUCT}" "${TARGET}" __TEMPLATE_RC ${__TEMPLATE_RC})
    string(REPLACE "{COMPANY}" "Tsinworks" __TEMPLATE_RC ${__TEMPLATE_RC})

    set(__RES_HEADER "\#pragma once\r\n")

    if(${TARGET}_ICON)
        string(APPEND __TEMPLATE_RC "#define IDI_ICON1 101\r\n")
        string(APPEND __TEMPLATE_RC "IDI_ICON1 ICON \"${${TARGET}_ICON}\"\r\n")
        string(APPEND __RES_HEADER "#define IDI_ICON1 101\r\n")
    endif()
    
    if(${TARGET}_ICON2)
        string(APPEND __TEMPLATE_RC "#define IDI_ICON2 102\r\n")
        string(APPEND __TEMPLATE_RC "IDI_ICON2 ICON \"${${TARGET}_ICON2}\"\r\n")
        string(APPEND __RES_HEADER "#define IDI_ICON2 102\r\n")
    endif()

    file(WRITE "${CMAKE_BINARY_DIR}/${TARGET}_WinRes.rc" "${__TEMPLATE_RC}")
    file(WRITE "${CMAKE_BINARY_DIR}/${TARGET}_resource.h" "${__RES_HEADER}")
    target_include_directories(${TARGET} PRIVATE ${CMAKE_BINARY_DIR})
    source_group(resource FILES "${CMAKE_BINARY_DIR}/${TARGET}_WinRes.rc" "${CMAKE_BINARY_DIR}/${TARGET}_resource.h")
    target_sources(${TARGET} PRIVATE "${CMAKE_BINARY_DIR}/${TARGET}_WinRes.rc" "${CMAKE_BINARY_DIR}/${TARGET}_resource.h")
endfunction()


function(add_ib_test TARGET)
    cmake_parse_arguments(${TARGET}
        ""
        "VER;DESC;ICON;ICON2"
        "SOURCES;LIBRARIES;"
        ${ARGN}
    )
    
    add_executable(${TARGET} ${${TARGET}_SOURCES})
    target_link_libraries(${TARGET} ${${TARGET}_LIBRARIES})
    set_target_properties(${TARGET} PROPERTIES 
        FOLDER "tests"
        CXX_STANDARD 20
        # COMPILE_PDB_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tests/pdb"
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tests/bin"
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
    )
endfunction()

if(WIN32)
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};C:/vcpkg/packages/ryml_x64-windows/share/ryml;C:/vcpkg/packages/llvm_x64-windows/share/clang;C:/vcpkg/packages/c4core_x64-windows/share/c4core")
endif()

#include(FindWDK.cmake)