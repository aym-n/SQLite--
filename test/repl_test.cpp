#include <catch2/catch_test_macros.hpp>
#include "repl.h"

TEST_CASE("Check toLowerCase", "[toLowerCase]") {
    InputBuffer* input = new InputBuffer();
    input->buffer = ".eXit";
    input->buffer_length = input->buffer.length();
    REQUIRE(toLowercase(input->buffer) == ".exit");
}

TEST_CASE("Check do_meta_command", "[do_meta_command]") {
    InputBuffer* input = new InputBuffer();
    input->buffer = ".hello";
    input->buffer_length = input->buffer.length();
    REQUIRE(do_meta_command(input) == META_COMMAND_UNRECOGNIZED_COMMAND);
}

TEST_CASE("failing test_case", "[fail]"){
    REQUIRE(1 == 2);
}