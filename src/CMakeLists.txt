include_directories(. common)
add_subdirectory(common)
add_subdirectory(coreui)
add_subdirectory(unittest)

# xcode substitute
if (WIN32)
	add_subdirectory(ycode)
endif()

if(BUILD_IGEN)
    add_subdirectory(igen)
endif()