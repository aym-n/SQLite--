#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <cstring>

#include "repl.h"

using namespace std;

InputBuffer::InputBuffer()
{
    input_buffer = 0;
    buffer_length = 0;
}

string toLowercase(const string &str)
{
    string result = str;
    for (char &c : result)
    {
        c = tolower(c);
    }
    return result;
}
void print_prompt()
{
    cout << " db > ";
}
void read_input(InputBuffer *input_buffer)
{
    getline(cin, input_buffer->buffer);
    input_buffer->buffer = toLowercase(input_buffer->buffer);
    input_buffer->buffer_length = input_buffer->buffer.length();
    if (!input_buffer->buffer.empty() && input_buffer->buffer[input_buffer->buffer_length - 1] == '\n')
    {
        input_buffer->buffer.pop_back(); // new line character remove kiya as it may effect the comparision process
        input_buffer->buffer_length--;   // as i am removing the last character so length kam hogi
    }
}
MetaCommandResult do_meta_command(InputBuffer *input_buffer)
{
    if (input_buffer->buffer == ".exit")
    {
        exit(EXIT_SUCCESS);
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}
Table* new_table() {
    Table* table = new Table;
    table->num_rows = 0;
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        table->pages[i] = nullptr;
    }
    return table;
}

void free_table(Table* table) {
    for (int i = 0; table->pages[i]; i++) {
       if (table->pages[i]) {
            delete[] static_cast<char*>(table->pages[i]);
            table->pages[i] = nullptr;
        }
    }
    delete table;
}
void close_input_buffer(InputBuffer* input_buffer) {
    delete input_buffer;
}

//till now no issue



void *row_slot(Table * table, uint32_t row_num) {
    uint32_t page_num = row_num / ROWS_PER_PAGE;  //eg row_num = 1000,ROWS_PER_PAGE = 499, page_num = 2
    void* page = table->pages[page_num]; //page_num = 2, table->pages[2] = 0x1234
    if (!page) {
        // Allocate memory only when we try to access page
        page = table->pages[page_num] = new char[PAGE_SIZE]; //page_num = 2, table->pages[2] = 0x1234
    }
    uint32_t row_offset = row_num % ROWS_PER_PAGE; 
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return (char*)page + byte_offset; //page = 0x1234, byte_offset = 1000, return 0x1234 + 1000
}
//checked
ExecuteResult execute_insert(Statement* statement, Table* table) {
    if (table->num_rows >= TABLE_MAX_ROWS) {
        return EXECUTE_TABLE_FULL;
    }

    Row* row_to_insert = &(statement->row_to_insert);

    serialize_row(row_to_insert, row_slot(table, table->num_rows));
    table->num_rows += 1;

    return EXECUTE_SUCCESS;
}
ExecuteResult execute_select(Statement* statement, Table* table) {
    Row row;
    for (uint32_t i = 0; i < table->num_rows; i++) {
        deserialize_row(row_slot(table, i), &row);
        cout<< '(' << row.id << " " << row.username << " " << row.email<< ')' << endl;
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement* statement, Table* table) {
    switch (statement->type) {
        case STATEMENT_INSERT:
            return execute_insert(statement, table);
        case STATEMENT_SELECT:
            return execute_select(statement, table);
        default:
            return EXECUTE_SUCCESS;
    }
}
PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement)
{
    istringstream input_buffer_string(input_buffer->buffer);
    string Buffer;
    input_buffer_string >> Buffer;
    if (Buffer == "insert")
    {
        statement->type = STATEMENT_INSERT;
        input_buffer_string >> statement->row_to_insert.id;
        input_buffer_string >> statement->row_to_insert.username;
        input_buffer_string >> statement->row_to_insert.email;
        // Extract values from the rest of the string(tricky part che ye)
        if (statement->row_to_insert.id == -1 || statement->row_to_insert.email.empty() || statement->row_to_insert.username.empty())
        {
            return PREPARE_SYNTAX_ERROR;
        }
        else
        {
            return PREPARE_SUCCESS;
        }
    }

    if (Buffer == "select")
    {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void serialize_row(Row* source, void* destination) {
    memcpy((char*)destination + ID_OFFSET, &(source->id), ID_SIZE);

    // Copy the username string
    char* dest_username = (char*)destination + USERNAME_OFFSET;
    strncpy(dest_username, source->username.c_str(), USERNAME_SIZE);
    dest_username[USERNAME_SIZE - 1] = '\0'; // Ensure null termination

    // Copy the email string
    char* dest_email = (char*)destination + EMAIL_OFFSET;
    strncpy(dest_email, source->email.c_str(), EMAIL_SIZE);
    dest_email[EMAIL_SIZE - 1] = '\0'; // Ensure null termination
}

void deserialize_row(void* source, Row* destination) {
    memcpy(&(destination->id), (char*)source + ID_OFFSET, ID_SIZE);

    // Copy the username string
    char* src_username = (char*)source + USERNAME_OFFSET;
    destination->username = src_username;

    // Copy the email string
    char* src_email = (char*)source + EMAIL_OFFSET;
    destination->email = src_email;
}