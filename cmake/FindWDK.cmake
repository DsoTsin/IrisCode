# Redistribution and use is allowed under the OSI-approved 3-clause BSD license.
# Copyright (c) 2018 Sergey Podobry (sergey.podobry at gmail.com). All rights reserved.

#.rst:
# FindWDK
# ----------
#
# This module searches for the installed Windows Development Kit (WDK) and
# exposes commands for creating kernel drivers and kernel libraries.
#
# Output variables:
# - `WDK_FOUND` -- if false, do not try to use WDK
# - `WDK_ROOT` -- where WDK is installed
# - `WDK_VERSION` -- the version of the selected WDK
# - `WDK_WINVER` -- the WINVER used for kernel drivers and libraries
#        (default value is `0x0601` and can be changed per target or globally)
# - `WDK_NTDDI_VERSION` -- the NTDDI_VERSION used for kernel drivers and libraries,
#                          if not set, the value will be automatically calculated by WINVER
#        (default value is left blank and can be changed per target or globally)
#
# Example usage:
#
#   find_package(WDK REQUIRED)
#
#   wdk_add_library(KmdfCppLib STATIC KMDF 1.15
#       KmdfCppLib.h
#       KmdfCppLib.cpp
#       )
#   target_include_directories(KmdfCppLib INTERFACE .)
#
#   wdk_add_driver(KmdfCppDriver KMDF 1.15
#       Main.cpp
#       )
#   target_link_libraries(KmdfCppDriver KmdfCppLib)
#

if(DEFINED ENV{WDKContentRoot})
    file(GLOB WDK_NTDDK_FILES
        "$ENV{WDKContentRoot}/Include/*/km/ntddk.h" # WDK 10
        "$ENV{WDKContentRoot}/Include/km/ntddk.h" # WDK 8.0, 8.1
    )
else()
    file(GLOB WDK_NTDDK_FILES
        "C:/Program Files*/Windows Kits/*/Include/*/km/ntddk.h" # WDK 10
        "C:/Program Files*/Windows Kits/*/Include/km/ntddk.h" # WDK 8.0, 8.1
    )
endif()

if(WDK_NTDDK_FILES)
    if (NOT CMAKE_VERSION VERSION_LESS 3.18.0)
        list(SORT WDK_NTDDK_FILES COMPARE NATURAL) # sort to use the latest available WDK
    endif()
    list(GET WDK_NTDDK_FILES -1 WDK_LATEST_NTDDK_FILE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WDK REQUIRED_VARS WDK_LATEST_NTDDK_FILE)

if (NOT WDK_LATEST_NTDDK_FILE)
    return()
endif()

get_filename_component(WDK_ROOT ${WDK_LATEST_NTDDK_FILE} DIRECTORY)
get_filename_component(WDK_ROOT ${WDK_ROOT} DIRECTORY)
get_filename_component(WDK_VERSION ${WDK_ROOT} NAME)
get_filename_component(WDK_ROOT ${WDK_ROOT} DIRECTORY)
if (NOT WDK_ROOT MATCHES ".*/[0-9][0-9.]*$") # WDK 10 has a deeper nesting level
    get_filename_component(WDK_ROOT ${WDK_ROOT} DIRECTORY) # go up once more
    set(WDK_LIB_VERSION "${WDK_VERSION}")
    set(WDK_INC_VERSION "${WDK_VERSION}")
else() # WDK 8.0, 8.1
    set(WDK_INC_VERSION "")
    foreach(VERSION winv6.3 win8 win7)
        if (EXISTS "${WDK_ROOT}/Lib/${VERSION}/")
            set(WDK_LIB_VERSION "${VERSION}")
            break()
        endif()
    endforeach()
    set(WDK_VERSION "${WDK_LIB_VERSION}")
endif()

message(STATUS "WDK_ROOT: " ${WDK_ROOT})
message(STATUS "WDK_VERSION: " ${WDK_VERSION})

set(WDK_SIGNTOOL ${WDK_ROOT}/bin/${WDK_VERSION}/x86/signtool.exe)
set(WDK_STAMPINF ${WDK_ROOT}/bin/${WDK_VERSION}/x86/stampinf.exe)
set(WDK_INF2CAT ${WDK_ROOT}/bin/${WDK_VERSION}/x86/inf2cat.exe)
set(WDK_MAKECERT ${WDK_ROOT}/bin/${WDK_VERSION}/x86/MakeCert.exe)

if (EXISTS "${WDK_SIGNTOOL}") 
    message(STATUS "SignTool = ${WDK_SIGNTOOL}")
endif()
if (EXISTS "${WDK_STAMPINF}")
    message(STATUS "StampInf = ${WDK_STAMPINF}")
endif()
if(EXISTS "${WDK_INF2CAT}")
    message(STATUS "Inf2Cat = ${WDK_INF2CAT}")
endif()
if(EXISTS "${WDK_MAKECERT}")
    message(STATUS "MakeCert = ${WDK_MAKECERT}")
endif()

set(WDK_WINVER "0x0601" CACHE STRING "Default WINVER for WDK targets")
set(WDK_NTDDI_VERSION "" CACHE STRING "Specified NTDDI_VERSION for WDK targets if needed")

set(WDK_ADDITIONAL_FLAGS_FILE "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/wdkflags.h")
file(WRITE ${WDK_ADDITIONAL_FLAGS_FILE} "#pragma runtime_checks(\"suc\", off)")

set(WDK_COMPILE_FLAGS
    "/Zp8" # set struct alignment
    "/GF"  # enable string pooling
    "/GR-" # disable RTTI
    "/Gz" # __stdcall by default
    "/kernel"  # create kernel mode binary
    "/FIwarning.h" # disable warnings in WDK headers
    "/FI${WDK_ADDITIONAL_FLAGS_FILE}" # include file to disable RTC
    )

set(WDK_COMPILE_DEFINITIONS "WINNT=1")
set(WDK_COMPILE_DEFINITIONS_DEBUG "MSC_NOOPT;DEPRECATE_DDK_FUNCTIONS=1;DBG=1")

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    list(APPEND WDK_COMPILE_DEFINITIONS "_X86_=1;i386=1;STD_CALL")
    set(WDK_PLATFORM "x86")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    if( "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64" )
        list(APPEND WDK_COMPILE_DEFINITIONS "_WIN64;_ARM64_")
        set(WDK_PLATFORM "arm64")
    else( )
        list(APPEND WDK_COMPILE_DEFINITIONS "_WIN64;_AMD64_;AMD64")
        set(WDK_PLATFORM "x64")
    endif( )
else()
    message(FATAL_ERROR "Unsupported architecture")
endif()

string(CONCAT WDK_LINK_FLAGS
    "/MANIFEST:NO " #
    "/DRIVER " #
    "/OPT:REF " #
    "/INCREMENTAL:NO " #
    "/OPT:ICF " #
    "/SUBSYSTEM:NATIVE " #
    "/MERGE:_TEXT=.text;_PAGE=PAGE " #
    "/NODEFAULTLIB " # do not link default CRT
    "/SECTION:INIT,d " #
    "/VERSION:10.0 " #
    )

# Generate imported targets for WDK lib files
file(GLOB WDK_LIBRARIES "${WDK_ROOT}/Lib/${WDK_LIB_VERSION}/km/${WDK_PLATFORM}/*.lib")
foreach(LIBRARY IN LISTS WDK_LIBRARIES)
    get_filename_component(LIBRARY_NAME ${LIBRARY} NAME_WE)
    string(TOUPPER ${LIBRARY_NAME} LIBRARY_NAME)
    add_library(WDK::${LIBRARY_NAME} INTERFACE IMPORTED)
    set_property(TARGET WDK::${LIBRARY_NAME} PROPERTY INTERFACE_LINK_LIBRARIES  ${LIBRARY})
endforeach(LIBRARY)
unset(WDK_LIBRARIES)

function(wdk_add_driver _target)
    cmake_parse_arguments(WDK "" "PFX;KMDF;WINVER;NTDDI_VERSION" "" ${ARGN})

    add_executable(${_target} ${WDK_UNPARSED_ARGUMENTS})

    set(INF_FILE "")   
    foreach(_FILE ${WDK_UNPARSED_ARGUMENTS})
        if (${_FILE} MATCHES ".*\.inf")
            set(INF_FILE "${_FILE}")
            break()
        endif()
    endforeach()

    set_target_properties(${_target} PROPERTIES SUFFIX ".sys")
    set_target_properties(${_target} PROPERTIES COMPILE_OPTIONS "${WDK_COMPILE_FLAGS}")
    set_target_properties(${_target} PROPERTIES COMPILE_DEFINITIONS
        "${WDK_COMPILE_DEFINITIONS};$<$<CONFIG:Debug>:${WDK_COMPILE_DEFINITIONS_DEBUG}>;_WIN32_WINNT=${WDK_WINVER}"
        )
    set_target_properties(${_target} PROPERTIES LINK_FLAGS "${WDK_LINK_FLAGS}")
    if(WDK_NTDDI_VERSION)
        target_compile_definitions(${_target} PRIVATE NTDDI_VERSION=${WDK_NTDDI_VERSION})
    endif()

    target_include_directories(${_target} SYSTEM PRIVATE
        "${WDK_ROOT}/Include/${WDK_INC_VERSION}/shared"
        "${WDK_ROOT}/Include/${WDK_INC_VERSION}/km"
        "${WDK_ROOT}/Include/${WDK_INC_VERSION}/km/crt"
        )

    if( "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64")
        target_link_directories(${_target} PRIVATE "${WDK_ROOT}/Lib/${WDK_LIB_VERSION}/um/${WDK_PLATFORM}/")    
        target_link_libraries(${_target} arm64rt WDK::NTOSKRNL WDK::HAL WDK::BUFFEROVERFLOWFASTFAILK WDK::BUFFEROVERFLOWGDI WDK::WMILIB WDK::USBDEX)
    else()
        target_link_libraries(${_target} WDK::NTOSKRNL WDK::HAL WDK::BUFFEROVERFLOWK WDK::WMILIB)
    endif()

    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_link_libraries(${_target} WDK::MEMCMP)
    endif()

    if(DEFINED WDK_KMDF)
        target_include_directories(${_target} SYSTEM PRIVATE "${WDK_ROOT}/Include/wdf/kmdf/${WDK_KMDF}")
        target_link_libraries(${_target}
            "${WDK_ROOT}/Lib/wdf/kmdf/${WDK_PLATFORM}/${WDK_KMDF}/WdfDriverEntry.lib"
            "${WDK_ROOT}/Lib/wdf/kmdf/${WDK_PLATFORM}/${WDK_KMDF}/WdfLdr.lib"
            )

        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set_property(TARGET ${_target} APPEND_STRING PROPERTY LINK_FLAGS "/ENTRY:FxDriverEntry@8")
        elseif(CMAKE_SIZEOF_VOID_P  EQUAL 8)
            set_property(TARGET ${_target} APPEND_STRING PROPERTY LINK_FLAGS "/ENTRY:FxDriverEntry")
        endif()
    else()
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set_property(TARGET ${_target} APPEND_STRING PROPERTY LINK_FLAGS "/ENTRY:GsDriverEntry@8")
        elseif(CMAKE_SIZEOF_VOID_P  EQUAL 8)
            set_property(TARGET ${_target} APPEND_STRING PROPERTY LINK_FLAGS "/ENTRY:GsDriverEntry")
        endif()
    endif()

    if (NOT "${INF_FILE}" STREQUAL "")    
        if( "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64" )
            add_custom_command(
                TARGET ${_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${_target}>/drivers/${_target}/"
                COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${_target}>" "$<TARGET_FILE_DIR:${_target}>/drivers/${_target}/"
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${INF_FILE}" "$<TARGET_FILE_DIR:${_target}>/drivers/${_target}/"
                COMMAND "${WDK_INF2CAT}" /os:Server10_arm64 /USELOCALTIME /driver:"$<TARGET_FILE_DIR:${_target}>/drivers/${_target}/"
                COMMAND ${CMAKE_COMMAND} -E remove "$<TARGET_FILE:${_target}>"
            )
        else()
            add_custom_command(
                TARGET ${_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${_target}>/drivers/${_target}/"
                COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${_target}>" "$<TARGET_FILE_DIR:${_target}>/drivers/${_target}/"
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${INF_FILE}" "$<TARGET_FILE_DIR:${_target}>/drivers/${_target}/"
                COMMAND "${WDK_INF2CAT}" /os:10_x64 /USELOCALTIME /driver:"$<TARGET_FILE_DIR:${_target}>/drivers/${_target}/"
                COMMAND ${CMAKE_COMMAND} -E remove "$<TARGET_FILE:${_target}>"
            )
        endif()
        if (NOT DEFINED WDK_PFX)
            # test signing
            # https://learn.microsoft.com/en-us/windows-hardware/drivers/install/test-signing
            add_custom_command(
                TARGET ${_target} POST_BUILD
                COMMAND "${WDK_MAKECERT}" -r -pe -ss PrivateCertStore -n "CN=Contoso.com(Test)" "$<TARGET_FILE_DIR:${_target}>/TestDriver.cer"
            )
            # Signtool sign /v /s PrivateCertStore /n Contoso.com(Test) /t http://timestamp.digicert.com tstamd64.cat
            add_custom_command(
                TARGET ${_target} POST_BUILD
                COMMAND "${WDK_SIGNTOOL}" sign /ph /fd "sha256" /a /v /s PrivateCertStore /n "Contoso.com(Test)" /t http://timestamp.digicert.com "$<TARGET_FILE_DIR:${_target}>/drivers/${_target}/${_target}.cat"
                COMMAND "${WDK_SIGNTOOL}" sign /ph /fd "sha256" /a /v /s PrivateCertStore /n "Contoso.com(Test)" /t http://timestamp.digicert.com "$<TARGET_FILE_DIR:${_target}>/drivers/${_target}/${_target}.sys"
            )
        else()
            add_custom_command(TARGET ${_target} POST_BUILD
            	COMMAND "${SIGNTOOL}" sign /v /f "${WDK_PFX}.pfx" /t http://timestamp.verisign.com/scripts/timstamp.dll $<TARGET_FILE:${_target}>
            	COMMENT "Signing $<TARGET_FILE:${_target} ...")
        endif()
    endif()
endfunction()

function(wdk_add_library _target)
    cmake_parse_arguments(WDK "" "KMDF;WINVER;NTDDI_VERSION" "" ${ARGN})

    add_library(${_target} ${WDK_UNPARSED_ARGUMENTS})

    set_target_properties(${_target} PROPERTIES COMPILE_OPTIONS "${WDK_COMPILE_FLAGS}")
    set_target_properties(${_target} PROPERTIES COMPILE_DEFINITIONS
        "${WDK_COMPILE_DEFINITIONS};$<$<CONFIG:Debug>:${WDK_COMPILE_DEFINITIONS_DEBUG};>_WIN32_WINNT=${WDK_WINVER}"
        )
    if(WDK_NTDDI_VERSION)
        target_compile_definitions(${_target} PRIVATE NTDDI_VERSION=${WDK_NTDDI_VERSION})
    endif()

    target_include_directories(${_target} SYSTEM PRIVATE
        "${WDK_ROOT}/Include/${WDK_INC_VERSION}/shared"
        "${WDK_ROOT}/Include/${WDK_INC_VERSION}/km"
        "${WDK_ROOT}/Include/${WDK_INC_VERSION}/km/crt"
        )

    if(DEFINED WDK_KMDF)
        target_include_directories(${_target} SYSTEM PRIVATE "${WDK_ROOT}/Include/wdf/kmdf/${WDK_KMDF}")
    endif()
endfunction()