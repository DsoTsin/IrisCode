get_filename_component(_mimalloc_prefix "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

set(_mimalloc_SOURCES
2.1.2/include/mimalloc-override.h
2.1.2/include/mimalloc-new-delete.h
2.1.2/include/mimalloc.h
2.1.2/include/mimalloc/atomic.h
2.1.2/include/mimalloc/internal.h
2.1.2/include/mimalloc/prim.h
2.1.2/include/mimalloc/track.h
2.1.2/include/mimalloc/types.h
2.1.2/src/alloc.c
2.1.2/src/alloc-aligned.c
2.1.2/src/alloc-posix.c
2.1.2/src/arena.c
2.1.2/src/bitmap.c
2.1.2/src/heap.c
2.1.2/src/init.c
2.1.2/src/options.c
2.1.2/src/os.c
2.1.2/src/page.c
2.1.2/src/random.c
2.1.2/src/segment.c
2.1.2/src/segment-map.c
2.1.2/src/stats.c
2.1.2/src/prim/prim.c
)

foreach(_src ${_mimalloc_SOURCES})
    list(APPEND mimalloc_SOURCES "${_mimalloc_prefix}/${_src}")
endforeach()

source_group("external\\mimalloc" FILES ${mimalloc_SOURCES})