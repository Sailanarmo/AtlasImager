project(AtlasLogger)

# In the WASM build AtlasLogger is header-only (ConsoleLogger writes to the
# browser console via emscripten_log).  The FileLogger .cpp is only compiled
# for native builds because it uses std::chrono::current_zone() / zoned_time.
if(AS_WASM)
  add_library(AtlasLogger INTERFACE)
  target_include_directories(AtlasLogger INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
else()
  add_library(AtlasLogger STATIC atlaslogger.cpp)
  target_include_directories(AtlasLogger PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} ${Qt6Core_INCLUDE_DIRS}
  )
  if(MSVC)
    target_compile_options(AtlasLogger PUBLIC "/Zc:__cplusplus")
  endif()
endif()

add_library(Atlas::Logger ALIAS AtlasLogger)
