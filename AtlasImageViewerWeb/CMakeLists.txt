project(AtlasImageViewerWeb)

add_library(AtlasImageViewerWeb STATIC atlasimageviewerweb.cpp)

target_include_directories(
  AtlasImageViewerWeb PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${ATLASIMAGER_ROOT_DIR}
  ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(AtlasImageViewerWeb PRIVATE Atlas::Image ${OpenCV_LIBS})

add_library(Atlas::ImageViewerWeb ALIAS AtlasImageViewerWeb)
