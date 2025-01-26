project(AtlasMessenger)

add_library(AtlasMessenger STATIC atlasmessenger.cpp)
target_include_directories(
  AtlasMessenger PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR} ${ATLASIMAGER_ROOT_DIR}
  ${Qt6OpenGL_INCLUDE_DIRS}
)

add_library(Atlas::Messenger ALIAS AtlasMessenger)