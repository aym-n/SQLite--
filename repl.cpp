#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>

using namespace std;



typedef struct{
    int id = -1;
    string username;
    string email;
}Row;
typedef enum
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum
{
    PREPARE_SUCCESS,
    PREPARE_SYNTAX_ERROR,
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
    Row row_to_insert;
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

//macros and other offsets
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)nullptr)->Attribute)
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;


string toLowercase(const string& str) {
    string result = str;
    for (char& c : result) {
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

PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement) {
    istringstream input_buffer_string(input_buffer->buffer);
    string Buffer;
    input_buffer_string >> Buffer;
    if (Buffer == "insert") {
        statement->type = STATEMENT_INSERT;
         input_buffer_string >> statement->row_to_insert.id ;
         input_buffer_string >> statement->row_to_insert.username ;
         input_buffer_string >> statement->row_to_insert.email;
        // Extract values from the rest of the string(tricky part che ye)
        if(statement->row_to_insert.id == -1 || statement->row_to_insert.email.empty() || statement->row_to_insert.username.empty()){
            return PREPARE_SYNTAX_ERROR;
        }
        else {
            return PREPARE_SUCCESS;
        }
    }

    if (Buffer == "select") {
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

        case (PREPARE_SYNTAX_ERROR):
	        printf("Syntax error. Could not parse statement.\n");
    	    continue;

        case (PREPARE_UNRECOGNIZED_STATEMENT):
            cout << "Unrecognized keyword at start of " << input_buffer->buffer << endl;
            continue;
        }

        execute_statement(&statement);
        cout << "Executed" << endl;
    }
    return 0;
}
