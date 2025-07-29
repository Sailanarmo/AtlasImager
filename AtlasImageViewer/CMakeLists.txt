project(AtlasImageViewer)

add_library(AtlasImageViewer STATIC atlasimageviewer.cpp)

target_include_directories(
  AtlasImageViewer PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR} ${ATLASIMAGER_ROOT_DIR}
  ${Qt6OpenGL_INCLUDE_DIRS}
)

target_link_libraries(AtlasImageViewer PRIVATE Qt6::OpenGL Qt6::Widgets)

if (MSVC)
  target_compile_options(AtlasImageViewer PUBLIC "/Zc:__cplusplus")
endif()


add_library(Atlas::ImageViewer ALIAS AtlasImageViewer)