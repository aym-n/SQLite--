#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "repl.h"
#include <strings.h>
#include "bTree.h"

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
MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table *table)
{
    if (input_buffer->buffer == ".exit")
    {
        db_close(table);
        exit(EXIT_SUCCESS);
    }
    else if (input_buffer->buffer == ".constants")
    {
        print_constants();
        return META_COMMAND_SUCCESS;
    }
    else if (input_buffer->buffer == ".btree")
    {
        cout << "Tree:\n";
        print_tree(table->pager, 0, 0);
        return META_COMMAND_SUCCESS;
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

Table *db_open(string db_file)
{
    Table *table = new Table;
    Pager *pager = new Pager(db_file);
    table->pager = pager;
    table->root_page_num = 0;

    if (pager->num_pages == 0)
    {
        // New database file. Initialize page 0 as leaf node.
        void *root_node = pager->get_page(0);
        initialize_leaf_node(root_node);
        set_node_root(root_node, true);
    }
    return table;
}

// Replaces the pager_open function
Pager::Pager(string db_file)
{
    // check if file exists
    FILE *file = fopen(db_file.c_str(), "r");
    if (!file)
    {
        // File does not exist, create and open it in read and write mode with truncate
        file = fopen(db_file.c_str(), "w+");
        if (file)
        {
            cout << "File created successfully." << std::endl;
            fclose(file);
        }
        else
        {
            cerr << "Error creating file." << std::endl;
            return;
        }
    }
    else
    {
        fclose(file);
    }
    // Now open the file in read and write mode
    file = fopen(db_file.c_str(), "r+");
    if (!file)
    {
        cout << "Could not open file" << endl;
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    uint32_t file_length = ftell(file);
    this->file_descriptor = file;
    this->file_length = file_length;
    this->num_pages = (file_length / PAGE_SIZE);
    if (file_length % PAGE_SIZE != 0)
    {
        printf("Db file is not a whole number of pages. Corrupt file.\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        this->pages[i] = nullptr;
    }
}

void close_input_buffer(InputBuffer *input_buffer)
{
    delete input_buffer;
}

// till now no issue
void *cursor_value(Cursor *cursor)
{
    uint32_t page_num = cursor->page_num;
    void *page = cursor->table->pager->get_page(page_num);
    return leaf_node_value(page, cursor->cell_num);
}
// checked
ExecuteResult execute_insert(Statement *statement, Table *table)
{
    void *node = table->pager->get_page(table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    Row *row_to_insert = &(statement->row_to_insert);
    uint32_t key_to_insert = row_to_insert->id;
    Cursor *cursor = table_find(table, key_to_insert);

    if(cursor->cell_num < num_cells){
        uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);
        if(key_at_index == key_to_insert){
            return EXECUTE_DUPLICATE_KEY;
        }
    }

    leaf_node_insert(cursor, row_to_insert->id, row_to_insert);

    delete cursor;
    return EXECUTE_SUCCESS;
}
ExecuteResult execute_select(Statement *statement, Table *table)
{
    Cursor *cursor = table_start(table);
    Row row;
    while (!cursor->end_of_table)
    {
        deserialize_row(cursor_value(cursor), &row);
        cout << '(' << row.id << " " << row.username << " " << row.email << ')' << endl;
        cursor_advance(cursor);
    }
    delete cursor;
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement *statement, Table *table)
{
    switch (statement->type)
    {
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

void serialize_row(Row *source, void *destination)
{
    memcpy((char *)destination + ID_OFFSET, &(source->id), ID_SIZE);

    // Copy the username string
    char *dest_username = (char *)destination + USERNAME_OFFSET;
    strncpy(dest_username, source->username.c_str(), USERNAME_SIZE);
    dest_username[USERNAME_SIZE - 1] = '\0'; // Ensure null termination

    // Copy the email string
    char *dest_email = (char *)destination + EMAIL_OFFSET;
    strncpy(dest_email, source->email.c_str(), EMAIL_SIZE);
    dest_email[EMAIL_SIZE - 1] = '\0'; // Ensure null termination
}

void deserialize_row(void *source, Row *destination)
{
    memcpy(&(destination->id), (char *)source + ID_OFFSET, ID_SIZE);

    // Copy the username string
    char *src_username = (char *)source + USERNAME_OFFSET;
    destination->username = src_username;

    // Copy the email string
    char *src_email = (char *)source + EMAIL_OFFSET;
    destination->email = src_email;
}

void *Pager::get_page(uint32_t page_num)
{
    if (page_num > TABLE_MAX_PAGES)
    {
        cout << "Tried to fetch page number out of bounds. " << page_num << " > " << TABLE_MAX_PAGES << endl;
        exit(EXIT_FAILURE);
    }

    if (this->pages[page_num] == nullptr)
    {
        // Cache miss. Allocate memory and load from file.
        void *page = malloc(PAGE_SIZE);
        uint32_t num_pages = this->file_length / PAGE_SIZE;

        // We might save a partial page at the end of the file
        if (this->file_length % PAGE_SIZE)
        {
            num_pages += 1;
        }

        if (page_num <= num_pages)
        {
            fseek(this->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            size_t bytes_read = fread(page, PAGE_SIZE, 1, this->file_descriptor);
            if (bytes_read == -1)
            {
                cout << "Error reading file: " << errno << endl;
                exit(EXIT_FAILURE);
            }
        }
        this->pages[page_num] = page;

        if (page_num >= this->num_pages)
        {
            this->num_pages = page_num + 1;
        }
    }

    return this->pages[page_num];
}

void db_close(Table *table)
{
    Pager *pager = table->pager;

    for (uint32_t i = 0; i < pager->num_pages; i++)
    {
        if (pager->pages[i] == nullptr)
        {
            continue;
        }
        pager->flush(i);
        free(pager->pages[i]);
        pager->pages[i] = nullptr;
    }

    int result = fclose(pager->file_descriptor);
    if (result == -1)
    {
        cout << "Error closing db file." << endl;
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        void *page = pager->pages[i];
        if (page)
        {
            free(page);
            pager->pages[i] = nullptr;
        }
    }

    delete pager;
    delete table;
}

void Pager::flush(uint32_t page_num)
{
    if (this->pages[page_num] == nullptr)
    {
        cout << "Tried to flush null page" << endl;
        exit(EXIT_FAILURE);
    }

    uint32_t offset = fseek(this->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
    if (offset == -1)
    {
        cout << "Error seeking: " << errno << endl;
        exit(EXIT_FAILURE);
    }

    size_t bytes_written = fwrite(this->pages[page_num], PAGE_SIZE, 1, this->file_descriptor);
    if (bytes_written == 0)
    {
        cout << "Error writing: " << errno << endl;
        exit(EXIT_FAILURE);
    }
}
Cursor *table_start(Table *table)
{
    Cursor *cursor = new Cursor;
    cursor->table = table;
    cursor->page_num = table->root_page_num;
    cursor->cell_num = 0;

    void *root_node = table->pager->get_page(table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->end_of_table = (num_cells == 0);
    return cursor;
}
void cursor_advance(Cursor *cursor)
{
    uint32_t page_num = cursor->page_num;
    void *node = cursor->table->pager->get_page(page_num);

    cursor->cell_num += 1;
    if (cursor->cell_num >= (*leaf_node_num_cells(node)))
    {
        cursor->end_of_table = true;
    }
}
Cursor* leaf_node_find(Table *table, uint32_t page_num, uint32_t key){
    void *node = table->pager->get_page(page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    Cursor *cursor = new Cursor;
    cursor->table = table;
    cursor->page_num = page_num;

    // Binary search
    uint32_t min_index = 0;
    uint32_t one_past_max_index = num_cells;
    while(one_past_max_index != min_index){
        uint32_t index = (min_index + one_past_max_index) / 2;
        uint32_t key_at_index = *leaf_node_key(node, index);
        if(key == key_at_index){
            cursor->cell_num = index;
            return cursor;
        }
        if(key < key_at_index){
            one_past_max_index = index;
        }else{
            min_index = index + 1;
        }
    }
    cursor->cell_num = min_index;
    return cursor;
}

Cursor *table_find(Table *table, uint32_t key)
{
    uint32_t root_page_num = table->root_page_num;
    void *root_node = table->pager->get_page(root_page_num);

    if (get_node_type(root_node) == NODE_LEAF)
    {
        return leaf_node_find(table, root_page_num, key);
    }
    else
    {
        return find_internal_node(table, root_page_num, key);
    }
}

void print_constants()
{
    cout << "ROW_SIZE: " << ROW_SIZE << endl;
    cout << "COMMON_NODE_HEADER_SIZE: " << COMMON_NODE_HEADER_SIZE << endl;
    cout << "LEAF_NODE_HEADER_SIZE: " << LEAF_NODE_HEADER_SIZE << endl;
    cout << "LEAF_NODE_CELL_SIZE: " << LEAF_NODE_CELL_SIZE << endl;
    cout << "LEAF_NODE_SPACE_FOR_CELLS: " << LEAF_NODE_SPACE_FOR_CELLS << endl;
    cout << "LEAF_NODE_MAX_CELLS: " << LEAF_NODE_MAX_CELLS << endl;
}

uint32_t Pager::get_unused_page_num(){
    return this->num_pages;
}

Cursor* find_internal_node(Table* table, uint32_t page_num, uint32_t key){
	void* node = table->pager->get_page(page_num);
	uint32_t num_keys = *internal_node_num_keys(node);

	int left = 0, right = num_keys;
	while(left != right){
		int index = (right + left) / 2;
		int current_key = *internal_node_key(node, index);

		if(current_key >= key) 
			right = index;
		else left = index + 1;
	}

	uint32_t num_child = *internal_node_child(node, left);
	void* child = table->pager->get_page(num_child);

	switch(get_node_type(child)){
	case NODE_LEAF:
		return leaf_node_find(table, num_child, key);
	case NODE_INTERNAL:
		return find_internal_node(table, num_child, key);
	}

}


