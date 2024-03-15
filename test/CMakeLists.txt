include(AddCatch2)

add_executable(
  replTests
  repl_test.cpp
)
target_link_libraries(
  replTests PRIVATE repl Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(replTests)