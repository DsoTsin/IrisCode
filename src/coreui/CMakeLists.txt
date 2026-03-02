set(CORE_UI_SOURCES
    uicore.hpp uidefs.hpp
    app.cc app.hpp
    view.cc view.hpp
    window.cc window.hpp
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
view/text_edit.hpp)
source_group("UI\\View" FILES ${CORE_UI_VIEW_SOURCES})
source_group(yoga FILES ${yoga_SOURCES})
add_library(core.ui ${CORE_UI_SOURCES} ${CORE_UI_VIEW_SOURCES} ${yoga_SOURCES})
add_library(ui SHARED ${CORE_UI_SOURCES} ${CORE_UI_VIEW_SOURCES} ${yoga_SOURCES})

if (WIN32)
    set(CORE_UI_WIN_SOURCE
    win/wincomp.hpp
    win/wincomp.cc
    win/wincomp_fx.hpp
    )
    source_group("UI\\Windows" FILES ${CORE_UI_WIN_SOURCE})
    target_sources(core.ui PRIVATE ${CORE_UI_WIN_SOURCE})
    target_sources(ui PRIVATE ${CORE_UI_WIN_SOURCE})
    target_link_libraries(core.ui skia::skia skia::spirvcross Usp10)
    target_link_libraries(ui skia::skia skia::spirvcross Usp10 common)
    target_compile_definitions(core.ui PRIVATE SK_DIRECT3D SK_GANESH SK_BUILD_FOR_WIN)
    target_compile_definitions(ui PRIVATE BUILD_SHARED BUILD_UICORE SK_DIRECT3D SK_GANESH SK_BUILD_FOR_WIN)
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