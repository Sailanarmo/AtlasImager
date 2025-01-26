cmake_minimum_required(VERSION 3.21)

project(AtlasImager)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(OpenCV REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL REQUIRED HINTS /opt/Qt/6.8.1/gcc_64)

qt_standard_project_setup()

set(ATLASIMAGER_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(AtlasGUI)
add_subdirectory(AtlasImage)
add_subdirectory(AtlasModel)
add_subdirectory(AtlasMessenger)
add_subdirectory(AtlasImageViewer)

qt_add_executable(AtlasImager atlasimager.cpp)
target_include_directories(AtlasImager PRIVATE ${ATLASIMAGER_ROOT_DIR})
target_link_libraries(
  AtlasImager PRIVATE ${OpenCV_LIBS} Atlas::Image 
  Qt6::Core Qt6::Gui Qt6::OpenGL Qt6::Widgets 
  Atlas::GUI Atlas::Messenger Atlas::Model Atlas::ImageViewer
)