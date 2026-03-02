cmake_minimum_required (VERSION 3.18)
project (IrisCode 
	LANGUAGES 
		C CXX ASM
)

option(BUILD_IGEN "build igen" OFF)
option(BUILD_WITH_BISON_FLEX "use bison & flex" OFF)
option(BUILD_WITH_UI "enable ui for iris build" ON)
option(ENABLE_SONY_PS "Enable Sony PlayStation Support" ON)
option(ENABLE_NINTENDO_SWITCH "Enable Nintendo Switch Support" ON)
option(ENABLE_SQLITE "Enable SQLite Support" ON)
option(ENABLE_TOOLS "Enable Tools" ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

if (MSVC)
	enable_language(ASM_MASM)
	set(CMAKE_ASM_MASM_SOURCE_FILE_EXTENSIONS asm)
endif()

include(config)
include(modules)

if(MSVC)
	add_definitions(/MP)
endif(MSVC)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ARCH_POSTFIX 32)
else()
    set(ARCH_POSTFIX "")
endif()

include_directories(
	include
	external/std.ext/include 
	external/rapidjson 
	external/tinyxml2
	external/detours/include
    external/libui
	external/mimalloc/2.1.2/include
	external/yoga
	external/
)

add_subdirectory(external/yoga/yoga)
add_subdirectory(src)
