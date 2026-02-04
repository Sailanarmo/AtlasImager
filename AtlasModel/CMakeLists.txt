project(Model)

add_library(Model STATIC model.cpp)

target_include_directories(Model PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR} ${ATLASIMAGER_ROOT_DIR} 
  ${OpenCV_INCLUDE_DIRS} ${Qt6OpenGL_INCLUDE_DIRS}
)

target_link_libraries(Model Atlas::Image)

if (MSVC)
  target_compile_options(Model PUBLIC "/Zc:__cplusplus")
endif()

add_library(Atlas::Model ALIAS Model)