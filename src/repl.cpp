#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>

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

void execute_statement(Statement *statement)
{

    switch (statement->type)
    {
    case (STATEMENT_INSERT):

        cout << "This is where we would do an insert." << endl;
        break;
    case (STATEMENT_SELECT):
        cout << "This is where we would do a select." << endl;
        break;
    }
}
