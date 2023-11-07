#include "btree.h"
#include "utils.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HEADER 4
#define POINTER 8
#define OFFSET 2
#define KEYVALUE 4

#define BTREE_PAGE_SIZE                                                        \
  4096 // TODO: change this to the actual disk page size (look it up)
#define BTREE_MAX_KEY_SIZE 1000
#define BTREE_MAX_VAL_SIZE 3000

void printKeyValue(KeyValue keyvalue) {
  uint16_t klen = keyvalue.klen;
  uint16_t vlen = keyvalue.vlen;
  printf("KeyLen %u, ValLen %u, Key %s, Value %s\n", klen, vlen,
         charArrayToString(keyvalue.key, klen),
         charArrayToString(keyvalue.value, vlen));
}

void printKeyValues(KeyValue *keyvalues, uint16_t nkeys) {
  printf("-----------KEYS-------------\n");
  if (keyvalues == NULL) {
    printf("NONE\n");
    return;
  }

  for (uint16_t i = 0; i < nkeys; i++) {
    printKeyValue(keyvalues[i]);
  }
  printf("----------------------------\n");
}

void printNode(Node *node) {
  printf("Type %d | Number of Keys %d\n", node->header.type,
         node->header.nkeys);
  printKeyValues(node->key_values, node->header.nkeys);
}

void printTree(BTree *tree) {
  printf("T: %d | Root offset %lu | Last Offset %lu\n", tree->t, tree->root,
         tree->last);
  printf("Root: \n");
  Node *root = nodeFromFile(tree->f, tree->root);
  if (root == NULL) {
    printf("No root yet");
  } else {
    printNode(root);
  }
}

KeyValue keyValueFromIndex(unsigned char *bytes, uint16_t index,
                           KeyOffset kvPos, uint16_t nkeys) {

  uint16_t klen = bytesToUInt16(bytes, kvPos);
  uint16_t vlen = bytesToUInt16(bytes, 2 + kvPos);

  char *key = malloc(klen * sizeof(char));
  char *value = malloc(vlen * sizeof(char));

  uint16_t keyStart = kvPos + 4;
  uint16_t valStart = kvPos + 4 + klen;

  for (uint16_t i = keyStart; i < valStart; i++) {
    key[i - keyStart] = bytes[i];
  }

  for (uint16_t i = valStart; i < valStart + vlen; i++) {
    value[i - valStart] = bytes[i];
  }

  KeyValue result;

  result.klen = klen;
  result.vlen = vlen;
  result.key = key;
  result.value = value;

  return result;
}

Node *nodeFromBytes(unsigned char *bytes) {
  Node *newNode = malloc(sizeof(Node));
  assert(newNode != NULL);

  NodeHeader newNodeHeader;

  newNodeHeader.type = bytesToUInt16(bytes, 0);

  newNodeHeader.nkeys = bytesToUInt16(bytes, 2);

  newNode->header = newNodeHeader;
  newNode->pointers = malloc(sizeof(NodePointer) * (newNodeHeader.nkeys + 1));
  newNode->offsets = malloc(sizeof(KeyOffset) * newNodeHeader.nkeys);
  newNode->key_values = malloc(sizeof(KeyValue) * newNodeHeader.nkeys);
  assert(newNode != NULL && newNode->pointers != NULL &&
         newNode->offsets != NULL && newNode->key_values != NULL);

  int pointersStart = HEADER;
  int offsetsStart = pointersStart + (newNodeHeader.nkeys + 1) * POINTER;
  int keysStart = offsetsStart + (newNodeHeader.nkeys) * OFFSET;

  // Load Pointers
  for (uint16_t i = 0; i < newNodeHeader.nkeys + 1; i++) {
    newNode->pointers[i] = bytesToUInt64(bytes, pointersStart + (POINTER * i));
  }

  for (uint16_t i = 0; i < newNodeHeader.nkeys; i++) {
    KeyOffset offset = bytesToUInt16(bytes, offsetsStart + (OFFSET * i));
    newNode->offsets[i] = offset;

    newNode->key_values[i] =
        keyValueFromIndex(bytes, i, keysStart + offset, newNodeHeader.nkeys);
  }

  return newNode;
}

int compare_key_value(const KeyValue kv1, const KeyValue kv2) {
  // Compare based on key, assuming keys are strings
  return strcmp(charArrayToString(kv1.key, kv1.klen),
                charArrayToString(kv2.key, kv2.klen));
}

void addKVtoNode(Node *node, KeyValue kv) {
  int i = node->header.nkeys - 1;

  // Reallocate key_values, offsets, and pointers
  node->key_values =
      realloc(node->key_values, (node->header.nkeys + 1) * sizeof(KeyValue));
  node->offsets =
      realloc(node->offsets, (node->header.nkeys + 1) * sizeof(KeyOffset));
  node->pointers =
      realloc(node->pointers, (node->header.nkeys + 2) * sizeof(NodePointer));

  // Shift key_values, offsets, and pointers to make space for new elements
  while (i >= 0 && compare_key_value(kv, node->key_values[i]) < 0) {
    node->key_values[i + 1] = node->key_values[i];
    node->offsets[i + 1] = node->offsets[i];
    node->pointers[i + 2] = node->pointers[i + 1];
    i--;
  }

  // Insert new key-value
  node->key_values[i + 1] = kv;

  // Recalculate offsets
  uint16_t currentBytes = 0;
  for (uint16_t j = 0; j <= node->header.nkeys; j++) {
    node->offsets[j] = currentBytes;
    currentBytes += 4; // Size of klen and vlen fields
    currentBytes += node->key_values[j].klen + node->key_values[j].vlen;
  }

  // Increment the number of keys
  node->header.nkeys += 1;
}

uint64_t nodeByteSize(Node *node) {
  uint64_t keyValueSize = 0;

  for (uint16_t i = 0; i < node->header.nkeys; i++) {
    keyValueSize += 4; // 2 bytes for klen and 2 bytes for vlen
    keyValueSize += node->key_values[i].klen;
    keyValueSize += node->key_values[i].vlen;
  }

  uint64_t nodeSize = HEADER + (node->header.nkeys + 1) * POINTER +
                      node->header.nkeys * OFFSET + keyValueSize;

  return nodeSize;
}

Node *nodeFromFile(FILE *db, uint64_t offset) {

  Node *newNode = malloc(sizeof(Node));
  if (newNode == NULL) {
    free(newNode);
    printf("Failed to malloc memory for node from file");
    return NULL;
  }

  if (fseek(db, offset, SEEK_SET) != 0) {
    free(newNode);
    return NULL;
  }

  unsigned char *headerBytes = malloc(HEADER * sizeof(char));
  if (fread(headerBytes, HEADER, 1, db) != 1) {
    free(newNode);
    printf("Failed to read the header");
    return 0;
  }

  NodeHeader header;
  header.type = bytesToUInt16(headerBytes, 0);
  header.nkeys = bytesToUInt16(headerBytes, 2);
  free(headerBytes);

  newNode->header = header;
  newNode->self_pointer = offset;

  newNode->pointers = malloc(sizeof(NodePointer) * (header.nkeys + 1));
  newNode->offsets = malloc(sizeof(KeyOffset) * header.nkeys);
  newNode->key_values = malloc(sizeof(KeyValue) * header.nkeys);

  unsigned char *pointerBytes =
      malloc(POINTER * sizeof(char) * (header.nkeys + 1));

  if (fread(pointerBytes, POINTER, header.nkeys + 1, db) != header.nkeys + 1) {
    // Handle read error or incomplete read
    printf("Failed while reading pointers\n");
    return 0;
  }
  for (uint16_t i = 0; i < header.nkeys + 1; i++) {
    newNode->pointers[i] = bytesToUInt64(pointerBytes, (POINTER * i));
  }
  free(pointerBytes);

  unsigned char *offsetBytes = malloc(OFFSET * sizeof(char) * header.nkeys);
  if (fread(offsetBytes, OFFSET, header.nkeys, db) != header.nkeys) {
    // Handle read error or incomplete read
    printf("Failed while reading offsets\n");
    return 0;
  }
  for (uint16_t i = 0; i < header.nkeys; i++) {
    newNode->offsets[i] = bytesToUInt16(offsetBytes, (OFFSET * i));
  }
  free(offsetBytes);

  unsigned char *keyvalueHeader = malloc(4 * sizeof(char));
  uint16_t klen, vlen;
  char *key, *value;
  KeyValue kvalue;

  for (uint16_t i = 0; i < header.nkeys; i++) {
    if (fread(keyvalueHeader, 4 * sizeof(char), 1, db) != 1) {
      printf("Failed while reading keyvalue header %d\n", i);
      return 0;
    }

    klen = bytesToUInt16(keyvalueHeader, 0);
    vlen = bytesToUInt16(keyvalueHeader, 2);

    key = malloc(klen * sizeof(char));
    value = malloc(vlen * sizeof(char));

    if (fread(key, klen * sizeof(char), 1, db) != 1) {
      // Handle read error or incomplete read
      printf("Failed while reading key %d\n", i);
      return 0;
    }

    if (fread(value, vlen * sizeof(char), 1, db) != 1) {
      // Handle read error or incomplete read
      printf("Failed while reading value %d\n", i);
      return 0;
    }

    kvalue.klen = klen;
    kvalue.vlen = vlen;
    kvalue.key = key;
    kvalue.value = value;

    newNode->key_values[i] = kvalue;
  }

  return newNode;
}

uint64_t nodeToBytes(Node *node, unsigned char **bytesPtr) {
  uint64_t nodeSize = nodeByteSize(node);
  unsigned char *bytes = malloc(nodeSize);

  if (bytes == NULL) {
    free(bytes);
    return 0; // Indicate an error
  }

  uint64_t currentByte = 0;

  uint16ToBytes(node->header.type, bytes, currentByte);
  uint16ToBytes(node->header.nkeys, bytes, currentByte + 2);

  currentByte += 4;

  for (uint16_t i = 0; i < node->header.nkeys + 1; i++) {
    uint64ToBytes(node->pointers[i], bytes, currentByte);
    currentByte += 8;
  }

  for (uint16_t i = 0; i < node->header.nkeys; i++) {
    uint16ToBytes(node->offsets[i], bytes, currentByte);
    currentByte += 2;
  }

  for (uint16_t i = 0; i < node->header.nkeys; i++) {
    uint16_t klen = node->key_values[i].klen;
    uint16_t vlen = node->key_values[i].vlen;

    uint16ToBytes(klen, bytes, currentByte);
    uint16ToBytes(vlen, bytes, currentByte + 2);

    currentByte += 4;

    memcpy(bytes + currentByte, node->key_values[i].key, klen);
    currentByte += klen;

    memcpy(bytes + currentByte, node->key_values[i].value, vlen);
    currentByte += vlen;
  }

  *bytesPtr = bytes; // Return the allocated memory to the caller
  return nodeSize;
}

void updateTreeInFile(BTree *tree) {
  if (fseek(tree->f, 0, SEEK_SET) != 0) { // Check for fseek error
    perror("fseek failed");
    exit(1);
  }

  // Write each field to the file
  if (fwrite(&tree->root, sizeof(NodePointer), 1, tree->f) != 1 ||
      fwrite(&tree->last, sizeof(NodePointer), 1, tree->f) != 1 ||
      fwrite(&tree->t, sizeof(uint16_t), 1, tree->f) != 1) {
    perror("fwrite failed");
    exit(1);
  }

  fflush(tree->f); // Flush the file buffer to ensure data is written
}

int addNodeToFile(BTree *tree, Node *node, NodePointer *destinationPointer) {

  // TODO: check if there's any available page before just adding to the end

  // Convert the node to bytes
  unsigned char *nodeBytes = NULL;
  uint64_t nodeSize = nodeToBytes(node, &nodeBytes);

  // Write the node bytes to the file
  if (fseek(tree->f, tree->last, SEEK_SET) != 0) {
    // Handle file seek error
    free(nodeBytes);
    return 0;
  }

  if (fwrite(nodeBytes, 1, nodeSize, tree->f) != nodeSize) {
    // Handle write error or incomplete write
    free(nodeBytes);
    return 0;
  }

  // Update the last pointer
  *destinationPointer = tree->last; // Point to the current last page
  tree->last += BTREE_PAGE_SIZE;    // Then update to the next page

  // Clean up
  free(nodeBytes);

  updateTreeInFile(tree);
  // Return success
  return 1;
}

int updateNodeOnFile(BTree *tree, Node *node) {
  unsigned char *nodeBytes = NULL;
  uint64_t nodeSize = nodeToBytes(node, &nodeBytes);

  if (nodeBytes == NULL) {
    return 0;
  }

  // Write the node bytes to the file
  if (fseek(tree->f, node->self_pointer, SEEK_SET) != 0) {
    // Handle file seek error
    free(nodeBytes);
    return 0;
  }

  if (fwrite(nodeBytes, 1, nodeSize, tree->f) != nodeSize) {
    // Handle write error or incomplete write
    free(nodeBytes);
    return 0;
  }

  free(nodeBytes);
  return 1;
}

BTree *treeFromFileName(char *filename) {

  BTree *result = malloc(sizeof(BTree));

  // TODO -> add more stuff, like loading the tree from the header of the file

  // Open the file for reading (change "r" to other modes for different
  // operations)
  FILE *file = fopen(filename, "r");

  // Check if the file was opened successfully
  if (file == NULL) {
    printf("Failed to open the file.\n");
    return NULL; // Return non-zero to indicate an error
  }

  result->f = file;
  result->root = 0;

  return result;
}

Node *createEmptyNode(uint16_t t) {
  Node *result = malloc(sizeof(Node));
  if (result == NULL) {
    perror("Memory allocation failed");
    exit(1);
  }

  result->header.nkeys = 0;
  result->header.type = INTERNAL;

  // Allocate memory for pointers, key values, and offsets based on the degree
  // (t)
  result->pointers = calloc((t + 1), sizeof(NodePointer));
  result->key_values = calloc(t, sizeof(KeyValue));
  result->offsets = calloc(t, sizeof(KeyOffset));

  if (result->pointers == NULL || result->key_values == NULL ||
      result->offsets == NULL) {
    perror("Memory allocation failed");
    free(result); // Clean up the partially allocated memory
    exit(1);
  }

  return result;
}

Node *createNode(int type, int t) {
  Node *new_node = (Node *)malloc(sizeof(Node));
  if (new_node == NULL) {
    exit(1); // Memory allocation error
  }

  new_node->header.type = type;
  new_node->header.nkeys = 0;
  new_node->pointers = (NodePointer *)malloc((2 * t) * sizeof(NodePointer));
  new_node->offsets =
      (uint16_t *)malloc((2 * t) * sizeof(uint16_t)); // Initialize offsets
  new_node->key_values = (KeyValue *)malloc((2 * t - 1) * sizeof(KeyValue));

  if (new_node->pointers == NULL || new_node->key_values == NULL ||
      new_node->offsets == NULL) {
    free(new_node->pointers);
    free(new_node->key_values);
    free(new_node->offsets);
    free(new_node);
    exit(1);
  }

  return new_node;
}

BTree *createMockupTree() {
  const char *filename = "/home/guilherme/Projetos/canonical/db.bin";
  FILE *file = fopen(filename, "w+b"); // Open in binary read-write mode

  if (file == NULL) {
    perror("Failed to open the file");
    return NULL;
  }

  BTree *result = malloc(sizeof(BTree));
  if (result == NULL) {
    perror("Memory allocation failed");
    fclose(file);
    return NULL;
  }

  result->f = file;
  result->root = BTREE_PAGE_SIZE;
  result->last = BTREE_PAGE_SIZE;
  result->t = 4;

  // Create an empty root node as a leaf
  Node *rootNode = createEmptyNode(result->t);
  rootNode->header.type = LEAF;

  KeyValue mock = {.klen = 1,
                   .vlen = 1,
                   .key = stringToCharArray("k"),
                   .value = stringToCharArray("v")};

  addKVtoNode(rootNode, mock);

  NodePointer destination;
  if (addNodeToFile(result, rootNode, &destination) != 1) {
    printf("ERROR WHILE CREATING MOCKUP TREE\n");
    fclose(file);
    free(result); // Clean up the allocated memory
    return NULL;
  }

  // Seek to the beginning of the file and update the root pointer
  // fseek(result->f, 0, SEEK_SET);
  // if (fwrite(&(result->root), sizeof(NodePointer), 1, result->f) != 1) {
  //  perror("Error writing root pointer");
  //  fclose(file);
  //  free(result); // Clean up the allocated memory
  //  return NULL;
  //}
  fseek(result->f, 0, SEEK_SET);
  updateTreeInFile(result);

  return result;
}

uint16_t getNextChild(Node *currentNode, char *key) {
  for (uint16_t i = 0; i < currentNode->header.nkeys + 1; i++) {
    KeyValue currentKeyvalue = currentNode->key_values[i];
    int cmp = memcmp(key, currentKeyvalue.key, currentKeyvalue.klen);
    if (cmp <= 0) {
      return i;
    } else {
      break;
    }
  }
  return 0;
}

int getKeyInNode(Node *node, char *key) {
  for (uint16_t i = 0; i < node->header.nkeys; i++) {
    KeyValue currentKeyvalue = node->key_values[i];
    int cmp = memcmp(key, currentKeyvalue.key, currentKeyvalue.klen);
    if (cmp == 0) {
      return i;
    }
  }

  return -1;
}

int searchKeyValue(BTree *tree, char *key, KeyValue *foundKv) {
  // Start searching from the root
  NodePointer currentPointer = tree->root;
  Node *currentNode = nodeFromFile(tree->f, currentPointer);

  while (currentNode != NULL) {

    int keyIndex = getKeyInNode(currentNode, key);
    if (keyIndex != -1) {
      KeyValue kv = currentNode->key_values[keyIndex];

      foundKv->klen = kv.klen;
      foundKv->vlen = kv.vlen;
      foundKv->key = kv.key;
      foundKv->value = kv.value;
      return 1;
    } else {
      if (currentNode->header.type == LEAF) {
        return -1;
      }
      uint16_t nextChildIndex = getNextChild(currentNode, key);
      currentPointer = currentNode->pointers[nextChildIndex];
      currentNode = nodeFromFile(tree->f, currentPointer);
    }
  }

  // Key not found
  return -1;
}

void splitChild(BTree *tree, Node *x, int i, int t) {
  Node *y = nodeFromFile(tree->f, x->pointers[i]);
  assert(y != NULL);

  Node *z = createNode(y->header.type, t);
  assert(z != NULL);

  // Move half of y's key-values and pointers to z
  for (int j = 0; j < t - 1; j++) {
    z->key_values[j] = y->key_values[j + t];
  }

  if (y->header.type == INTERNAL) {
    for (int j = 0; j < t; j++) {
      z->pointers[j] = y->pointers[j + t];
    }
  }

  // Update y's number of keys
  y->header.nkeys = t - 1;

  // Shift x's pointers and key-values to make room for new elements
  for (int j = x->header.nkeys; j >= i + 1; j--) {
    x->pointers[j + 1] = x->pointers[j];
  }
  for (int j = x->header.nkeys - 1; j >= i; j--) {
    x->key_values[j + 1] = x->key_values[j];
  }

  // Insert median key-value from y to x
  x->key_values[i] = y->key_values[t - 1];
  x->header.nkeys++;

  // Update x's pointers to include z
  NodePointer destinationZ;
  if (addNodeToFile(tree, z, &destinationZ) != 1) {
    exit(1);
  }
  x->pointers[i + 1] = destinationZ;

  // Update nodes in file
  updateNodeOnFile(tree, x);
  updateNodeOnFile(tree, y);
  updateNodeOnFile(tree, z);
}

void insertNonFull(BTree *tree, Node *x, KeyValue key_value) {
  int i = x->header.nkeys - 1;
  if (x->header.type == LEAF) {
    addKVtoNode(x, key_value);
    updateNodeOnFile(tree, x);
  } else {
    while (i >= 0 && compare_key_value(key_value, x->key_values[i]) < 0) {
      i--;
    }
    i++;
    // Load the child node pointed to by x->pointers[i]
    Node *child = nodeFromFile(tree->f, x->pointers[i]);
    if (child->header.nkeys == (2 * tree->t) - 1) {
      // The child is full, split it
      splitChild(tree, x, i, tree->t);
      // Decide which of the two children to descend to
      if (compare_key_value(key_value, x->key_values[i]) > 0) {
        i++;
      }
      child = nodeFromFile(tree->f, x->pointers[i]);
    }
    insertNonFull(tree, child, key_value);
  }
}

void insert(BTree *tree, KeyValue key_value) {
  Node *root = nodeFromFile(tree->f, tree->root);
  assert(root != NULL);

  if (root->header.nkeys == (2 * tree->t) - 1) {
    Node *new_root = createNode(INTERNAL, tree->t);
    assert(new_root != NULL);

    new_root->pointers[0] = tree->root;

    splitChild(tree, new_root, 0, tree->t);
    insertNonFull(tree, new_root, key_value);

    NodePointer newRootPointer;
    if (addNodeToFile(tree, new_root, &newRootPointer) != 1) {
      exit(1);
    }

    tree->root = newRootPointer;
    updateTreeInFile(tree);
  } else {
    insertNonFull(tree, root, key_value);
  }
}

int readKeyValuePairs(const char *filename, KeyValue **resultPtr) {

  char *separator = ":";
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    perror("Failed to open file");
    return -1;
  }

  int count = 0;
  KeyValue *result = NULL;

  char line[1024];
  while (fgets(line, sizeof(line), file)) {
    char *token = strtok(line, separator);
    if (token != NULL) {
      KeyValue kv;

      char *key = strdup(token);
      kv.klen = strlen(key);
      kv.key = malloc(kv.klen + 1);
      strcpy(kv.key, key);
      free(key);

      token = strtok(NULL, separator);
      if (token != NULL) {
        char *value = strdup(token);
        kv.vlen = strlen(value);
        kv.value = malloc(kv.vlen + 1);
        strcpy(kv.value, value);
        free(value);

        result = realloc(result, (count + 1) * sizeof(KeyValue));
        result[count] = kv;
        count++;
      }
    }
  }

  fclose(file);
  *resultPtr = result;

  return count;
}
