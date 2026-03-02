get_filename_component(yoga_prefix "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(_yoga_SOURCES
  yoga/YGConfig.cpp
  yoga/YGEnums.cpp
  yoga/YGNode.cpp
  yoga/YGNodeLayout.cpp
  yoga/YGNodeStyle.cpp
  yoga/YGPixelGrid.cpp
  yoga/YGValue.cpp
  yoga/algorithm/AbsoluteLayout.cpp
  yoga/algorithm/Baseline.cpp
  yoga/algorithm/Cache.cpp
  yoga/algorithm/CalculateLayout.cpp
  yoga/algorithm/FlexLine.cpp
  yoga/algorithm/PixelGrid.cpp
  yoga/config/Config.cpp
  yoga/debug/AssertFatal.cpp
  yoga/debug/Log.cpp
  yoga/event/event.cpp
  yoga/node/LayoutResults.cpp
  yoga/node/Node.cpp
)

foreach(_src ${_yoga_SOURCES})
    list(APPEND yoga_SOURCES "${yoga_prefix}/${_src}")
endforeach()

source_group("external\\yoga" FILES ${yoga_SOURCES})