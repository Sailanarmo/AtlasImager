cmake_minimum_required(VERSION 3.21)

project(AtlasImager)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(OpenGL REQUIRED)
find_package(OpenCV REQUIRED HINTS ${CMAKE_CURRENT_LIST_DIR}/thirdparty/opencv/install)
find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL REQUIRED)

qt_standard_project_setup()

set(ATLASIMAGER_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(AtlasGUI)
add_subdirectory(AtlasImage)
add_subdirectory(AtlasModel)
add_subdirectory(AtlasMessenger)
add_subdirectory(AtlasImageViewer)

set(WindowsIconResource "${CMAKE_CURRENT_LIST_DIR}/resources/atlasimager.rc")

if(MSVC)
  qt_add_executable(AtlasImager atlasimager.cpp ${WindowsIconResource})
else()
  qt_add_executable(AtlasImager atlasimager.cpp)
endif()

target_include_directories(AtlasImager PRIVATE ${ATLASIMAGER_ROOT_DIR})
target_link_libraries(
  AtlasImager PUBLIC ${OpenCV_LIBS} OpenGL::GL 
  Qt6::Core Qt6::Gui Qt6::OpenGL Qt6::Widgets Atlas::Image
  Atlas::GUI Atlas::Messenger Atlas::Model Atlas::ImageViewer
)

if (MSVC)
  target_compile_options(AtlasImager PUBLIC "/Zc:__cplusplus")
  set_target_properties(AtlasImager PROPERTIES WIN32_EXECUTABLE TRUE)

  add_custom_command(
    TARGET AtlasImager
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AtlasImager>/platforms"
  )

  add_custom_command(
    TARGET AtlasImager
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_RUNTIME_DLLS:AtlasImager>
      $<TARGET_FILE_DIR:AtlasImager>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${Qt6_DIR}/../../../plugins/platforms/qwindows.dll"
      "$<TARGET_FILE_DIR:AtlasImager>/platforms/qwindows.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${Qt6_DIR}/../../../plugins/imageformats/qtiff.dll"
      "$<TARGET_FILE_DIR:AtlasImager>/plugins/imageformats/qtiff.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_CURRENT_LIST_DIR}/AtlasModel/Dataset
      $<TARGET_FILE_DIR:AtlasImager>/Dataset
    COMMAND_EXPAND_LISTS # Important for expanding the list of DLLs
    COMMENT "Copying runtime DLLs and Dataset to executable directory..."
  )
endif()