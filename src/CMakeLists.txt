add_library(libs repl.cpp bTree.cpp)
target_include_directories(libs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})