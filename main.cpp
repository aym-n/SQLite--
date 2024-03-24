#include <iostream>
#include "src/repl.h"

using namespace std;

int main(int argc, char *argv[])
{   
    if (argc < 2)
    {
        cout << "Must supply a database filename." << endl;
        exit(EXIT_FAILURE);
    }

    string db_file = argv[1];
    Table* table = db_open(db_file);

    InputBuffer *input_buffer= new InputBuffer;
    while (true)
    {
        print_prompt();
        read_input(input_buffer);
        if (input_buffer->buffer[0] == '.')
        {
            switch (do_meta_command(input_buffer, table))
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
        
         switch (execute_statement(&statement, table))
        {
        case (EXECUTE_SUCCESS):
            cout << "Executed" << endl;
            break;
        case (EXECUTE_TABLE_FULL):
            cout << "Error: Table full." << endl;
            break;
        }
    }
    
    return 0;
}