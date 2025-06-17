project(AtlasGUI)

add_library(AtlasGUI STATIC
  atlascontrolwidget_layout.cpp
  atlascontrolwidget.cpp
  atlasmainwindow_layout.cpp
  atlasmainwindow.cpp
  opacitySlider.cpp
)

target_include_directories(
  AtlasGUI PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR} ${ATLASIMAGER_ROOT_DIR}
  ${Qt6Widgets_INCLUDE_DIRS} ${Qt6OpenGL_INCLUDE_DIRS}
)

add_library(Atlas::GUI ALIAS AtlasGUI)