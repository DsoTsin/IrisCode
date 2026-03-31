set(CORE_UI_SOURCES
    uicore.hpp uidefs.hpp
    app.cc app.hpp
    view.cc view.hpp
    window.cc window.hpp
    dock_window.cc dock_window.hpp
    graphics_context.cc graphics_context.hpp
)
source_group(UI FILES ${CORE_UI_SOURCES})
set(CORE_UI_VIEW_SOURCES
view/label.cc
view/label.hpp
view/button.cc
view/button.hpp
view/check_box.cc
view/check_box.hpp
view/text_input.cc
view/text_input.hpp
view/list_view.cc
view/list_view.hpp
view/table_view.cc
view/table_view.hpp
view/dock_view.cc
view/dock_view.hpp
view/text_edit.cc
view/text_edit.hpp
view/code_editor.cc
view/code_editor.hpp)
source_group("UI\\View" FILES ${CORE_UI_VIEW_SOURCES})
source_group(yoga FILES ${yoga_SOURCES})
add_library(core.ui ${CORE_UI_SOURCES} ${CORE_UI_VIEW_SOURCES} ${yoga_SOURCES})
add_library(ui SHARED ${CORE_UI_SOURCES} ${CORE_UI_VIEW_SOURCES} ${yoga_SOURCES})

if (WIN32)
    set(CORE_UI_WIN_SOURCE
    platform/win/win32_app.hpp
    platform/win/win32_app.cc
    platform/win/win32_graphics_context.hpp
    platform/win/win32_graphics_context.cc
    platform/win/win32_window.hpp
    platform/win/win32_window.cc
    platform/win/wincomp.hpp
    platform/win/wincomp.cc
    platform/win/wincomp_fx.hpp
    )
    source_group("UI\\Windows" FILES ${CORE_UI_WIN_SOURCE})
    target_sources(core.ui PRIVATE ${CORE_UI_WIN_SOURCE})
    target_sources(ui PRIVATE ${CORE_UI_WIN_SOURCE})
    target_link_libraries(core.ui skia::skia skia::spirvcross Usp10)
    target_link_libraries(ui skia::skia skia::spirvcross Usp10 common)
    target_compile_definitions(core.ui PRIVATE SK_DIRECT3D SK_GANESH SK_BUILD_FOR_WIN)
    target_compile_definitions(ui PRIVATE BUILD_SHARED BUILD_UICORE SK_DIRECT3D SK_GANESH SK_BUILD_FOR_WIN)
endif()

if (APPLE)
    set(CORE_UI_APPLE_SOURCE
    platform/apple/appl_graphics_context.hpp
    platform/apple/appl_graphics_context.cc
    platform/apple/appl_window.hpp
    platform/apple/appl_window.cc
    )
    source_group("UI\\Apple" FILES ${CORE_UI_APPLE_SOURCE})
    target_sources(core.ui PRIVATE ${CORE_UI_APPLE_SOURCE})
    target_sources(ui PRIVATE ${CORE_UI_APPLE_SOURCE})
endif()


set_target_properties(core.ui PROPERTIES 
    FOLDER "irisbuild" 
    CXX_STANDARD 20
    MSVC_RUNTIME_LIBRARY "MultiThreaded"
)

set_target_properties(ui PROPERTIES 
    FOLDER "irisbuild" 
    CXX_STANDARD 20
    MSVC_RUNTIME_LIBRARY "MultiThreaded"
)
