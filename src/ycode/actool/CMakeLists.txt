set(EXT_SOURCES
    ${CMAKE_SOURCE_DIR}/external/bom/bom.c
    ${CMAKE_SOURCE_DIR}/external/bom/bom_tree.c
    ${CMAKE_SOURCE_DIR}/external/bom/bom_memory.c

    ${CMAKE_SOURCE_DIR}/external/libplist/src/Array.cpp
    ${CMAKE_SOURCE_DIR}/external/libplist/src/base64.c
    ${CMAKE_SOURCE_DIR}/external/libplist/src/Boolean.cpp
    ${CMAKE_SOURCE_DIR}/external/libplist/src/bplist.c
    ${CMAKE_SOURCE_DIR}/external/libplist/src/bytearray.c
    ${CMAKE_SOURCE_DIR}/external/libplist/src/Data.cpp
    ${CMAKE_SOURCE_DIR}/external/libplist/src/Date.cpp
    ${CMAKE_SOURCE_DIR}/external/libplist/src/Dictionary.cpp
    ${CMAKE_SOURCE_DIR}/external/libplist/src/hashtable.c
    ${CMAKE_SOURCE_DIR}/external/libplist/src/Integer.cpp
    ${CMAKE_SOURCE_DIR}/external/libplist/src/Key.cpp
    ${CMAKE_SOURCE_DIR}/external/libplist/src/Node.cpp
    ${CMAKE_SOURCE_DIR}/external/libplist/src/plist.c
    ${CMAKE_SOURCE_DIR}/external/libplist/src/ptrarray.c
    ${CMAKE_SOURCE_DIR}/external/libplist/src/Real.cpp
    ${CMAKE_SOURCE_DIR}/external/libplist/src/String.cpp
    ${CMAKE_SOURCE_DIR}/external/libplist/src/Structure.cpp
    ${CMAKE_SOURCE_DIR}/external/libplist/src/time64.c
    ${CMAKE_SOURCE_DIR}/external/libplist/src/Uid.cpp
    ${CMAKE_SOURCE_DIR}/external/libplist/src/xplist.c
    ${CMAKE_SOURCE_DIR}/external/libplist/libcnary/node.c
    ${CMAKE_SOURCE_DIR}/external/libplist/libcnary/node_list.c
    ${CMAKE_SOURCE_DIR}/external/libplist/libcnary/cnary.c

    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzfse_decode.c
    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzfse_decode_base.c
    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzfse_encode.c
    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzfse_encode_base.c
    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzfse_fse.c
    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzvn_decode_base.c
    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzvn_encode_base.c
)
source_group(ext FILES ${EXT_SOURCES})

set(ASCAT_SRCS
asset_catalog.h
asset_catalog.cpp
ycore_config.h
compressor.h
compressor.cpp
)

source_group(car FILES ${ASCAT_SRCS})

if (MSVC)
    set(API_DEF "YCORE_API=__declspec(dllexport)")
else()
    set(API_DEF "YCORE_API=__attribute__((visibility(\"default\")))")
endif()

yc_add_lib(ycore SHARED
    SOURCES
        ${EXT_SOURCES}
        ${ASCAT_SRCS}
    LIBS
        ZLIB::ZLIB
    DEFS
        ${API_DEF}
    INCS
        ${CMAKE_SOURCE_DIR}/external
        ${CMAKE_SOURCE_DIR}/external/libplist/include
        ${CMAKE_SOURCE_DIR}/external/libplist/libcnary/include
        ${CMAKE_SOURCE_DIR}/external/liblzfse/src
    )

yc_add_bin(actool 
    SOURCES actool_main.cpp driver.cpp driver.h
    LIBS ycore
)