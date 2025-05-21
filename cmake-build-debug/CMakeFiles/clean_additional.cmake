# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "AtlasGUI/AtlasGUI_autogen"
  "AtlasGUI/CMakeFiles/AtlasGUI_autogen.dir/AutogenUsed.txt"
  "AtlasGUI/CMakeFiles/AtlasGUI_autogen.dir/ParseCache.txt"
  "AtlasImage/CMakeFiles/Image_autogen.dir/AutogenUsed.txt"
  "AtlasImage/CMakeFiles/Image_autogen.dir/ParseCache.txt"
  "AtlasImage/Image_autogen"
  "AtlasImageViewer/AtlasImageViewer_autogen"
  "AtlasImageViewer/CMakeFiles/AtlasImageViewer_autogen.dir/AutogenUsed.txt"
  "AtlasImageViewer/CMakeFiles/AtlasImageViewer_autogen.dir/ParseCache.txt"
  "AtlasImager_autogen"
  "AtlasMessenger/AtlasMessenger_autogen"
  "AtlasMessenger/CMakeFiles/AtlasMessenger_autogen.dir/AutogenUsed.txt"
  "AtlasMessenger/CMakeFiles/AtlasMessenger_autogen.dir/ParseCache.txt"
  "AtlasModel/CMakeFiles/Model_autogen.dir/AutogenUsed.txt"
  "AtlasModel/CMakeFiles/Model_autogen.dir/ParseCache.txt"
  "AtlasModel/Model_autogen"
  "CMakeFiles/AtlasImager_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/AtlasImager_autogen.dir/ParseCache.txt"
  )
endif()
