#ifndef BTREE_H
#define BTREE_H

#include <stdint.h>
#include <stdio.h>

typedef enum nodeType { INTERNAL, LEAF, DELETED } nodeType;

typedef struct NodeHeader {
  nodeType type;  // Type of the node (leaf node or internal node)
  uint16_t nkeys; // Number of keys
} NodeHeader;

typedef uint64_t NodePointer; // Pointer to child node. This refers disk
                              // pointers and not memory pointers

typedef uint16_t KeyOffset; // Offset to key-value pair

/*
| klen | vlen | key | val |
|  2B  |  2B  | ... | ... |
*/
typedef struct KeyValue {
  uint16_t klen;
  uint16_t vlen;
  char *key;
  char *value;
} KeyValue;

/*
| type | nkeys |  pointers        |   offsets  | key-values
|  2B  |   2B  | (nkeys + 1) * 8B | nkeys * 2B | ...
*/
typedef struct Node {
  struct NodeHeader header;
  NodePointer self_pointer;
  NodePointer *pointers;
  KeyOffset *offsets;
  KeyValue *key_values;
} Node;

// TODO: Create destroyer
typedef struct BTree {
  NodePointer root;
  NodePointer last;
  FILE *f;
  uint16_t t;
} BTree;

Node *nodeFromBytes(unsigned char *bytes);
Node *nodeFromFile(FILE *db, uint64_t offset);
BTree *treeFromFileName(char *filename);
int searchKeyValue(BTree *tree, char *key, KeyValue *foundKv);

BTree *createMockupTree();
void insert(BTree *tree, KeyValue key_value);
void printTree(BTree *tree);
void printKeyValues(KeyValue *keyvalues, uint16_t nkeys);
int readKeyValuePairs(const char *filename, KeyValue **resultPtr);
void printKeyValue(KeyValue keyvalue);

#endif // BTREE_H
