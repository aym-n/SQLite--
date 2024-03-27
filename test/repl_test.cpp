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
    Table* table = db_open("test.db");
    REQUIRE(do_meta_command(input, table) == META_COMMAND_UNRECOGNIZED_COMMAND);
}

TEST_CASE("check prepare_statement", "[prepare_statement]") {
    InputBuffer* input = new InputBuffer();
    input->buffer = "insert 1 user1 person1";
    input->buffer_length = input->buffer.length();
    Statement statement;
    REQUIRE(prepare_statement(input, &statement) == PREPARE_SUCCESS);
}

