project(AtlasMessenger)

add_library(AtlasMessenger STATIC atlasmessenger.cpp)
target_include_directories(
  AtlasMessenger PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR} ${ATLASIMAGER_ROOT_DIR}
  ${Qt6OpenGL_INCLUDE_DIRS}
)

if(MSVC)
  target_compile_options(AtlasMessenger PUBLIC "/Zc:__cplusplus")
endif()

add_library(Atlas::Messenger ALIAS AtlasMessenger)