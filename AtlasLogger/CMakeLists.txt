project(AtlasLogger)

add_library(AtlasLogger STATIC atlaslogger.cpp)
target_include_directories(
  AtlasLogger PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MSVC)
  target_compile_options(AtlasLogger PUBLIC "/Zc:__cplusplus")
endif()

add_library(Atlas::Logger ALIAS AtlasLogger)