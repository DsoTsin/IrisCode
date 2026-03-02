set(SKIA_OS ${CMAKE_SYSTEM_NAME})
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
set(SKIA_OS "macos")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
set(SKIA_OS "windows")
endif()
fetchcontent_declare(skia_${CMAKE_SYSTEM_NAME}_${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}_Release
    URL https://github.com/JetBrains/skia-pack/releases/download/m144-e2e6623374-4/Skia-m144-e2e6623374-4-${SKIA_OS}-Release-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}.zip
    #URL_HASH SHA256=${hash}
)
fetchcontent_declare(skia_${CMAKE_SYSTEM_NAME}_${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}_Debug
    URL https://github.com/JetBrains/skia-pack/releases/download/m144-e2e6623374-4/Skia-m144-e2e6623374-4-${SKIA_OS}-Debug-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}.zip
    #URL_HASH SHA256=${hash}
)
if(NOT skia_${CMAKE_SYSTEM_NAME}_${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}_Release_POPULATED)
  fetchContent_makeavailable(skia_${CMAKE_SYSTEM_NAME}_${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}_Release)
endif()
if(NOT skia_${CMAKE_SYSTEM_NAME}_${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}_Debug_POPULATED)
  fetchContent_makeavailable(skia_${CMAKE_SYSTEM_NAME}_${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}_Debug)
endif()

message(STATUS "skia_${CMAKE_SYSTEM_NAME}_${CMAKE_CXX_COMPILER_ARCHITECTURE_ID} ${skia_${CMAKE_SYSTEM_NAME}_${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}_Release_SOURCE_DIR}")
add_library(skia::skia STATIC IMPORTED)
add_library(skia::spirvcross STATIC IMPORTED)

fetchContent_GetProperties(
  skia_${CMAKE_SYSTEM_NAME}_${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}_Release
  SOURCE_DIR sk_rel_dir
)
fetchContent_GetProperties(
  skia_${CMAKE_SYSTEM_NAME}_${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}_Debug
  SOURCE_DIR sk_dbg_dir
)

message(STATUS "SKIA = ${sk_rel_dir}")
if(WIN32)
set_target_properties(skia::skia PROPERTIES 
    IMPORTED_LOCATION_RELEASE "${sk_rel_dir}/out/Release-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/skia.lib"
    IMPORTED_LOCATION_MINSIZEREL "${sk_rel_dir}/out/Release-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/skia.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO "${sk_rel_dir}/out/Release-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/skia.lib"
    IMPORTED_LOCATION_DEBUG "${sk_dbg_dir}/out/Debug-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/skia.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${sk_rel_dir}/"
)
set_target_properties(skia::spirvcross PROPERTIES 
    IMPORTED_LOCATION_RELEASE "${sk_rel_dir}/out/Release-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/spirv_cross.lib"
    IMPORTED_LOCATION_MINSIZEREL "${sk_rel_dir}/out/Release-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/spirv_cross.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO "${sk_rel_dir}/out/Release-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/spirv_cross.lib"
    IMPORTED_LOCATION_DEBUG "${sk_dbg_dir}/out/Debug-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/spirv_cross.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${sk_rel_dir}/"
)
else()
set_target_properties(skia::skia PROPERTIES 
    IMPORTED_LOCATION_RELEASE "${sk_rel_dir}/out/Release-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/libskia.a"
    IMPORTED_LOCATION_MINSIZEREL "${sk_rel_dir}/out/Release-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/libskia.a"
    IMPORTED_LOCATION_RELWITHDEBINFO "${sk_rel_dir}/out/Release-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/libskia.a"
    IMPORTED_LOCATION_DEBUG "${sk_dbg_dir}/out/Debug-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/libskia.a"
    INTERFACE_INCLUDE_DIRECTORIES "${sk_rel_dir}/"
)
set_target_properties(skia::spirvcross PROPERTIES 
    IMPORTED_LOCATION_RELEASE "${sk_rel_dir}/out/Release-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/libspirv_cross.a"
    IMPORTED_LOCATION_MINSIZEREL "${sk_rel_dir}/out/Release-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/libspirv_cross.a"
    IMPORTED_LOCATION_RELWITHDEBINFO "${sk_rel_dir}/out/Release-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/libspirv_cross.a"
    IMPORTED_LOCATION_DEBUG "${sk_dbg_dir}/out/Debug-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/libspirv_cross.a"
    INTERFACE_INCLUDE_DIRECTORIES "${sk_rel_dir}/"
)
include_directories(${sk_rel_dir})
endif()