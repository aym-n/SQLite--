#include "bTree.h"

#include <string.h>
#include <sys/types.h>

#include <cstdint>
#include <iostream>

#include "repl.h"

using namespace std;

uint32_t *leaf_node_num_cells(void *node) { return (uint32_t *)((char *)node + LEAF_NODE_NUM_CELLS_OFFSET); }

void *leaf_node_cell(void *node, uint32_t cell_num) { return (void *)node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE; }

uint32_t *leaf_node_key(void *node, uint32_t cell_num) { return (uint32_t *)leaf_node_cell(node, cell_num); }

uint32_t *node_parent(void *node) { return (uint32_t *)((char *)node + PARENT_POINTER_OFFSET); }

void *leaf_node_value(void *node, uint32_t cell_num) { return (void *)leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE; }

uint32_t *leaf_node_next_leaf(void *node) { return (uint32_t *)((char *)node + LEAF_NODE_NEXT_LEAF_OFFSET); }

NodeType get_node_type(void *node)
{
  uint8_t value = *((uint8_t *)((char *)node + NODE_TYPE_OFFSET));
  return (NodeType)value;
}
void set_node_type(void *node, NodeType type)
{
  uint8_t value = type;
  *((uint8_t *)((char *)node + NODE_TYPE_OFFSET)) = value;
}

void initialize_leaf_node(void *node)
{
  set_node_type(node, NODE_LEAF);
  set_node_root(node, false);
  *leaf_node_num_cells(node) = 0;
  *leaf_node_next_leaf(node) = 0;
}

void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value)
{
  void *node = cursor->table->pager->get_page(cursor->page_num);

  uint32_t num_cells = *leaf_node_num_cells(node);
  if (num_cells >= LEAF_NODE_MAX_CELLS)
  {
    // Node full
    leaf_node_split_and_insert(cursor, key, value);
    return;
  }

  if (cursor->cell_num < num_cells)
  {
    // Make room for new cell
    for (uint32_t i = num_cells; i > cursor->cell_num; i--)
      memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
  }

  *(leaf_node_num_cells(node)) += 1;
  *(leaf_node_key(node, cursor->cell_num)) = key;
  serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

void indent(uint32_t level)
{
  for (uint32_t i = 0; i < level; i++) cout << "  ";
}

void print_tree(Pager *pager, uint32_t page_num, uint32_t indentation_level)
{
  void *node = pager->get_page(page_num);
  uint32_t num_keys, child;

  switch (get_node_type(node))
  {
    case NODE_LEAF:
      num_keys = *leaf_node_num_cells(node);
      indent(indentation_level);
      cout << "- leaf (size " << num_keys << ")\n";
      for (uint32_t i = 0; i < num_keys; i++)
      {
        indent(indentation_level + 1);
        cout << "- " << *leaf_node_key(node, i) << "\n";
      }
      break;
    case NODE_INTERNAL:
      num_keys = *internal_node_num_keys(node);
      indent(indentation_level);
      cout << "- internal (size " << num_keys << ")\n";
      for (uint32_t i = 0; i < num_keys; i++)
      {
        child = *internal_node_child(node, i);
        print_tree(pager, child, indentation_level + 1);

        indent(indentation_level + 1);
        cout << "- key " << *internal_node_key(node, i) << "\n";
      }
      child = *internal_node_right_child(node);
      print_tree(pager, child, indentation_level + 1);
      break;
  }
}

void leaf_node_split_and_insert(Cursor *cursor, uint32_t key, Row *value)
{
  /*
    Create a new node and move half the cells over.
    Insert the new value in one of the two nodes.
    Update parent or create a new parent.
    */

  void *old_node = cursor->table->pager->get_page(cursor->page_num);

  uint32_t old_max = get_node_max_key(old_node);
  uint32_t new_page_num = cursor->table->pager->get_unused_page_num();
  void *new_node = cursor->table->pager->get_page(new_page_num);
  initialize_leaf_node(new_node);

  *node_parent(new_node) = *node_parent(old_node);
  *leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
  *leaf_node_next_leaf(old_node) = new_page_num;

  /*
    All existing keys plus new key should be divided
    evenly between old (left) and new (right) nodes.
    Starting from the right, move each key to correct position.
    */

  for (int i = LEAF_NODE_MAX_CELLS; i >= 0; i--)
  {
    void *destination_node;
    if (i >= LEAF_NODE_LEFT_SPLIT_COUNT)
      destination_node = new_node;
    else
      destination_node = old_node;
    uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
    void *destination = leaf_node_cell(destination_node, index_within_node);

    if (i == cursor->cell_num)
    {
      serialize_row(value, leaf_node_value(destination_node, index_within_node));
      *leaf_node_key(destination_node, index_within_node) = key;
    }
    else if (i > cursor->cell_num)
    {
      memcpy(destination, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
    }
    else
    {
      memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
    }
  }

  /*
   Update cell count on both leaf nodes
   */

  *(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
  *(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;

  if (is_node_root(old_node))
  {
    return create_new_root(cursor->table, new_page_num);
  }
  else
  {
    uint32_t parent_page_num = *node_parent(old_node);
    uint32_t new_max = get_node_max_key(old_node);
    void *parent = cursor->table->pager->get_page(parent_page_num);

    update_internal_node_key(parent, old_max, new_max);
    internal_node_insert(cursor->table, parent_page_num, new_page_num);
    return;
  }
}

void create_new_root(Table *table, uint32_t right_child_page_num)
{
  /*
    Handle splitting the root.
    Old root copied to new page, becomes left child.
    Address of right child passed in.
    Re-init root page to contain the new root node.
    New root node points to two children.
    */

  void *root = table->pager->get_page(table->root_page_num);
  void *right_child = table->pager->get_page(right_child_page_num);
  uint32_t left_child_page_num = table->pager->get_unused_page_num();
  void *left_child = table->pager->get_page(left_child_page_num);

  /*
    Left child has data copied from old root
    */

  memcpy(left_child, root, PAGE_SIZE);
  set_node_root(left_child, false);

  /*
    Root node is a new internal node with one key and two children.
    */

  initialize_internal_node(root);
  set_node_root(root, true);
  *(internal_node_num_keys(root)) = 1;
  *internal_node_child(root, 0) = left_child_page_num;
  uint32_t left_child_max_key = get_node_max_key(left_child);
  *internal_node_key(root, 0) = left_child_max_key;
  *internal_node_right_child(root) = right_child_page_num;

  *node_parent(left_child) = table->root_page_num;
  *node_parent(right_child) = table->root_page_num;
}

uint32_t *internal_node_num_keys(void *node) { return (uint32_t *)((char *)node + INTERNAL_NODE_NUM_KEYS_OFFSET); }

uint32_t *internal_node_right_child(void *node) { return (uint32_t *)((char *)node + INTERNAL_NODE_RIGHT_CHILD_OFFSET); }

uint32_t *internal_node_cell(void *node, uint32_t cell_num)
{
  return (uint32_t *)((char *)node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE);
}

uint32_t *internal_node_child(void *node, uint32_t child_num)
{
  uint32_t num_keys = *internal_node_num_keys(node);
  if (child_num > num_keys)
  {
    printf("Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
    exit(EXIT_FAILURE);
  }
  else if (child_num == num_keys)
  {
    return internal_node_right_child(node);
  }
  else
  {
    return internal_node_cell(node, child_num);
  }
}

uint32_t *internal_node_key(void *node, uint32_t key_num) { return (uint32_t *)((char *)internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE); }

uint32_t get_node_max_key(void *node)
{
  switch (get_node_type(node))
  {
    case NODE_LEAF:
      return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
    case NODE_INTERNAL:
      return *internal_node_key(node, *internal_node_num_keys(node) - 1);
  }
}

bool is_node_root(void *node)
{
  uint8_t value = *((uint8_t *)((char *)node + IS_ROOT_OFFSET));
  return (bool)value;
}

void set_node_root(void *node, bool is_root)
{
  uint8_t value = is_root;
  *((uint8_t *)((char *)node + IS_ROOT_OFFSET)) = value;
}

void initialize_internal_node(void *node)
{
  set_node_type(node, NODE_INTERNAL);
  set_node_root(node, false);
  *internal_node_num_keys(node) = 0;
}

uint32_t find_internal_node_child(void *node, uint32_t key)
{
  uint32_t num_keys = *internal_node_num_keys(node);

  uint32_t left = 0, right = num_keys;
  while (left != right)
  {
    int index = (right + left) / 2;
    int current_key = *internal_node_key(node, index);

    if (current_key >= key)
      right = index;
    else
      left = index + 1;
  }

  return left;
}

Cursor *internal_node_find(Table *table, uint32_t page_num, uint32_t key)
{
  void *node = table->pager->get_page(page_num);
  uint32_t child_index = find_internal_node_child(node, key);
  uint32_t num_child = *internal_node_child(node, child_index);
  void *child = table->pager->get_page(num_child);

  switch (get_node_type(child))
  {
    case NODE_LEAF:
      return leaf_node_find(table, num_child, key);
    case NODE_INTERNAL:
      return internal_node_find(table, num_child, key);
  }
}

void internal_node_insert(Table *table, uint32_t parent_page_num, uint32_t child_page_num)
{
  /*
  Add a new child/key pair to parent that corresponds to child
  */

  void *parent = table->pager->get_page(parent_page_num);
  void *child = table->pager->get_page(child_page_num);
  uint32_t child_max_key = get_node_max_key(child);
  uint32_t index = find_internal_node_child(parent, child_max_key);

  uint32_t original_num_keys = *internal_node_num_keys(parent);
  *internal_node_num_keys(parent) = original_num_keys + 1;

  if (original_num_keys >= INTERNAL_NODE_MAX_CELLS)
  {
    printf("Need to implement splitting internal node\n");
    exit(EXIT_FAILURE);
  }
  uint32_t right_child_page_num = *internal_node_right_child(parent);
  void *right_child = table->pager->get_page(right_child_page_num);

  if (child_max_key > get_node_max_key(right_child))
  {
    /* Replace right child */
    *internal_node_child(parent, original_num_keys) = right_child_page_num;
    *internal_node_key(parent, original_num_keys) = get_node_max_key(right_child);
    *internal_node_right_child(parent) = child_page_num;
  }
  else
  {
    /* Make room for the new cell */
    for (uint32_t i = original_num_keys; i > index; i--)
    {
      void *destination = internal_node_cell(parent, i);
      void *source = internal_node_cell(parent, i - 1);
      memcpy(destination, source, INTERNAL_NODE_CELL_SIZE);
    }
    *internal_node_child(parent, index) = child_page_num;
    *internal_node_key(parent, index) = child_max_key;
  }
}

void update_internal_node_key(void *node, uint32_t old_key, uint32_t new_key)
{
  uint32_t old_child_index = find_internal_node_child(node, old_key);
  *internal_node_key(node, old_child_index) = new_key;
}
