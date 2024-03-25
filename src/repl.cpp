#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <cstdio>

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
MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table* table)
{
    if (input_buffer->buffer == ".exit")
    {   
        db_close(table);
        exit(EXIT_SUCCESS);
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}


Table* db_open(string db_file) {
    Table* table = new Table;
    Pager* pager = new Pager(db_file);
    table->pager = pager;

    uint32_t num_rows = pager->file_length / ROW_SIZE;
    table->num_rows = num_rows;
    return table;
}


// Replaces the pager_open function
Pager::Pager(string db_file){
    FILE* file = fopen(db_file.c_str(), "r+"); // Create a new file if the file doesn't exist already
    if(!file){
        cout << "Could not open file" << endl;
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    uint32_t file_length = ftell(file);
    this->file_descriptor = file;
    this->file_length = file_length;
}

void close_input_buffer(InputBuffer* input_buffer) {
    delete input_buffer;
}

//till now no issue
void *row_slot(Table * table, uint32_t row_num) {
    uint32_t page_num = row_num / ROWS_PER_PAGE;  //eg row_num = 1000,ROWS_PER_PAGE = 499, page_num = 2
    void* page = table->pager->get_page(page_num); //page = 0x1234
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

void* Pager::get_page(uint32_t page_num){
    if(page_num > TABLE_MAX_PAGES){
        cout << "Tried to fetch page number out of bounds. " << page_num << " > " << TABLE_MAX_PAGES << endl;
        exit(EXIT_FAILURE);
    }

    if(this->pages[page_num] == nullptr){
        // Cache miss. Allocate memory and load from file.
        void* page = malloc(PAGE_SIZE);
        uint32_t num_pages = this->file_length / PAGE_SIZE;

        // We might save a partial page at the end of the file
        if(this->file_length % PAGE_SIZE){
            num_pages += 1;
        }

        if(page_num <= num_pages){
            fseek(this->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            size_t bytes_read = fread(page, PAGE_SIZE, 1, this->file_descriptor);
            if(bytes_read == -1){
                cout << "Error reading file: " << errno << endl;
                exit(EXIT_FAILURE);
            }
        }
        this->pages[page_num] = page;
    }

    return this->pages[page_num];
}

void db_close(Table* table){
    Pager* pager = table->pager;
    uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

    for(uint32_t i = 0; i < num_full_pages; i++){
        if(pager->pages[i] == nullptr){
            continue;
        }
        pager->flush(i, PAGE_SIZE);
        free(pager->pages[i]);
        pager->pages[i] = nullptr;
    }

    // There may be a partial page to write to the end of the file
    uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
    if(num_additional_rows > 0){
        uint32_t page_num = num_full_pages;
        if(pager->pages[page_num] != nullptr){
            pager->flush(page_num, num_additional_rows * ROW_SIZE);
            free(pager->pages[page_num]);
            pager->pages[page_num] = nullptr;
        }
    }

    int result = fclose(pager->file_descriptor);
    if(result == -1){
        cout << "Error closing db file." << endl;
        exit(EXIT_FAILURE);
    }

    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++){
        void* page = pager->pages[i];
        if(page){
            free(page);
            pager->pages[i] = nullptr;
        }
    }

    delete pager;
    delete table;
}

void Pager::flush(uint32_t page_num, uint32_t size){
    if(this->pages[page_num] == nullptr){
        cout << "Tried to flush null page" << endl;
        exit(EXIT_FAILURE);
    }

    uint32_t offset = fseek(this->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
    if(offset == -1){
        cout << "Error seeking: " << errno << endl;
        exit(EXIT_FAILURE);
    }

    size_t bytes_written = fwrite(this->pages[page_num], size, 1, this->file_descriptor);
    if(bytes_written == 0){
        cout << "Error writing: " << errno << endl;
        exit(EXIT_FAILURE);
    }
}