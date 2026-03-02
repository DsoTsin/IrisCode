get_filename_component(lzfse_prefix "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

set(_lzfse_SOURCES
  src/lzfse_decode.c
  src/lzfse_decode_base.c
  src/lzfse_encode.c
  src/lzfse_encode_base.c
  src/lzfse_fse.c
  src/lzvn_decode_base.c
  src/lzvn_encode_base.c
)

foreach(_src ${_lzfse_SOURCES})
    list(APPEND lzfse_SOURCES "${lzfse_prefix}/${_src}")
endforeach()

source_group("external\\lzfse" FILES ${lzfse_SOURCES})