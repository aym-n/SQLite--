#include <iostream>
#include <string>

using namespace std;
typedef enum
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum
{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef struct
{
    StatementType type;
} Statement;

class InputBuffer
{
public:
    string buffer;
    size_t buffer_length;
    ssize_t input_buffer;

    InputBuffer()
    {
        input_buffer = 0;
        buffer_length = 0;
    }
};
void print_prompt()
{
    cout << " db > ";
}
void read_input(InputBuffer *input_buffer)
{
    getline(cin, input_buffer->buffer);
    input_buffer->buffer_length = input_buffer->buffer.length();
    if (!input_buffer->buffer.empty() && input_buffer->buffer[input_buffer->buffer_length - 1] == '\n')
    {
        input_buffer->buffer.pop_back(); // new line character remove kiya as it may effect the comparision process
        input_buffer->buffer_length--;   // as i am removing the last character so length kam hogi
    }
}
MetaCommandResult do_meta_command(InputBuffer *input_buffer)
{
    if (input_buffer->buffer == ".exit" || input_buffer->buffer == ".EXIT")
    {
        exit(EXIT_SUCCESS);
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

PrepareResult prepare_statement(InputBuffer *input_buffer,
                                Statement *statement)
{
    if (input_buffer->buffer == "insert" || input_buffer->buffer == "INSERT")
    {
        statement->type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }
    if (input_buffer->buffer == "select" || input_buffer->buffer == "SELECT")
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
        cout << "This is where we would do an insert."<<endl;
        break;
    case (STATEMENT_SELECT):
        cout << "This is where we would do a select."<<endl;
        break;
    }
}

int main()
{
    InputBuffer *input_buffer= new InputBuffer;
    while (true)
    {
        print_prompt();
        read_input(input_buffer);
        if (input_buffer->buffer[0] == '.')
        {
            switch (do_meta_command(input_buffer))
            {
            case (META_COMMAND_SUCCESS):
                continue;
            case (META_COMMAND_UNRECOGNIZED_COMMAND):
                cout << "Unrecognized command" << input_buffer->buffer << endl;
                continue;
            }
        }

        Statement statement;
        switch (prepare_statement(input_buffer, &statement))
        {
        case (PREPARE_SUCCESS):
            break;
        case (PREPARE_UNRECOGNIZED_STATEMENT):
            cout << "Unrecognized keyword at start of " << input_buffer->buffer << endl;
            continue;
        }

        execute_statement(&statement);
        cout << "Executed" << endl;
    }
    return 0;
}
