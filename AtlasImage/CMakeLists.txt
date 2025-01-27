project(Image)

add_library(Image STATIC image.cpp)

target_include_directories(Image PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(Image ${OpenCV_LIBRARIES})

add_library(Atlas::Image ALIAS Image)