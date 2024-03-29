#ifndef REPL_H
#define REPL_H

#include <string>
#include <cstdint>

using namespace std;

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
typedef struct {
  uint32_t id;
  string username;
  string email;
} Row;

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)nullptr)->Attribute)
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

//macros for the pointer table to the pages
const uint32_t PAGE_SIZE = 4096;
const uint32_t TABLE_MAX_PAGES = 100;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

class Pager {
public:
  FILE* file_descriptor;
  uint32_t file_length;
  uint32_t num_pages;
  void* pages[TABLE_MAX_PAGES];

  Pager(string db_file); // Replaces open_pager function 
  void flush(uint32_t page_num);
  
  void* get_page(uint32_t page_num);
};
typedef struct {
  uint32_t num_rows;
  uint32_t root_page_num;
  Pager* pager;
} Table;

class Cursor {
public:
  Table* table;
  uint32_t row_num;
  bool end_of_table;

};


typedef enum MetaCommandResult {
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND,
} MetaCommandResult;

typedef enum ExecuteResult {
  EXECUTE_SUCCESS,
  EXECUTE_TABLE_FULL,
} ExecuteResult;


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
MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table* table);
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement);
ExecuteResult execute_insert(Statement* statement, Table* table);
ExecuteResult execute_select(Statement* statement, Table* table);
ExecuteResult execute_statement(Statement* statement, Table* table);
void serialize_row(Row* source, void* destination);
void deserialize_row(void* source, Row* destination);
Cursor* table_start(Table* table);
Cursor* table_end(Table* table);
void * cursor_value(Cursor* cursor);
void cursor_advance(Cursor* cursor);
Table* db_open(string db_file);

void db_close(Table* table);

#endif

