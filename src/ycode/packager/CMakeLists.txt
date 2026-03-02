set(EXT_SOURCES
    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzfse_decode.c
    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzfse_decode_base.c
    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzfse_encode.c
    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzfse_encode_base.c
    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzfse_fse.c
    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzvn_decode_base.c
    ${CMAKE_SOURCE_DIR}/external/liblzfse/src/lzvn_encode_base.c
)
source_group(ext FILES ${EXT_SOURCES})

yc_add_bin(
ypack
    SOURCES
        packager.cpp ${EXT_SOURCES}
    INCS
        "${CMAKE_SOURCE_DIR}/external/liblzfse/src/"
)