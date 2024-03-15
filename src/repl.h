#ifndef REPL_H
#define REPL_H

#include <string>
#include <cstdint>

using namespace std;

typedef struct Row {
  int id = -1;
  string username;
  string email;
} Row;

typedef enum MetaCommandResult {
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND,
} MetaCommandResult;

typedef enum PrepareResult {
  PREPARE_SUCCESS,
  PREPARE_SYNTAX_ERROR,
  PREPARE_UNRECOGNIZED_STATEMENT,
} PrepareResult;

typedef enum StatementType {
  STATEMENT_INSERT,
  STATEMENT_SELECT,
} StatementType;

typedef struct Statement {
  StatementType type;
  Row row_to_insert;
} Statement;

// Define InputBuffer class for managing user input

class InputBuffer {
public:
  string buffer;
  size_t buffer_length;
  ssize_t input_buffer;

  InputBuffer();
};

// Function prototypes for functionalities defined in repl.cpp
string toLowercase(const string& str);
void print_prompt();
void read_input(InputBuffer* input_buffer);
MetaCommandResult do_meta_command(InputBuffer *input_buffer);
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement);
void execute_statement(Statement* statement);


// Define macros for offsets within the Row struct

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)nullptr)->Attribute)
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

#endif

