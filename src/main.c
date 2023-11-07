#include "btree.h"
#include "utils.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_BYTES_SIZE
unsigned char SAMPLE_BYTES[SAMPLE_BYTES_SIZE] = {
    0x00, 0x01, // Node type
    0x00, 0x02, // Number of keys

    0x00, 0x00, 0x01, 0x01, 0x09, 0x12, 0x16, 0x18, // Pointer 1
    0x00, 0x00, 0x01, 0x01, 0x09, 0x15, 0x16, 0x18, // Pointer 1
    0x00, 0x00, 0x01, 0x01, 0x09, 0x19, 0x16, 0x18, // Pointer 1

    0x00, 0x00, // Offset 1
    0x00, 0x0e, // Offset 2

    0x00, 0x04,                        // Key 1 length
    0x00, 0x06,                        // Value 1 length
    'k',  'e',  'y',  '1',             // Key 1
    'v',  'a',  'l',  'u',  'e',  '1', // Value 1

    0x00, 0x05,                                   // Key 2 length
    0x00, 0x08,                                   // Value 2 length
    'k',  'e',  'y',  '2',  '2',                  // Key 2
    'v',  'a',  'l',  'u',  'e',  '2',  '2',  '2' // Value 2
};

/*
unsigned char SAMPLE_BYTES[SAMPLE_BYTES_SIZE] = {
0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x09, 0x12, 0x16, 0x18, 0x00,
0x00, 0x01, 0x01, 0x09, 0x15, 0x16, 0x18, 0x00, 0x00, 0x01, 0x01, 0x09, 0x19,
0x16, 0x18, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x04, 0x00, 0x06, 0x6b, 0x65, 0x79,
0x31, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x31, 0x00, 0x05, 0x00, 0x08, 0x6b, 0x65,
0x79, 0x32, 0x32, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x32, 0x32, 0x32
};

*/

int main() {
  BTree *tree = createMockupTree();
  if (tree == NULL) {
    printf("COULDN?T CREATE MOCKUP TREE");
    return -1;
  }

  KeyValue *result = NULL;

  int count = readKeyValuePairs(
      "/home/guilherme/Projetos/canonical/keyvaluetests.txt", &result);

  if (count <= 0) {
    printf("Failed to read file");
    exit(0);
  }

  for (int i = 0; i < count; i++) {
    insert(tree, result[i]);
  }

  KeyValue foundKV;
  for (int i = 0; i < count; i++) {
    int wasFound = searchKeyValue(tree, result[i].key, &foundKV);
    assert(wasFound && strcmp(charArrayToString(foundKV.value, foundKV.vlen),
                              foundKV.key) == 0);
  }
}

int oldtest() {
  BTree *tree = createMockupTree();
  if (tree == NULL) {
    printf("COULDN?T CREATE MOCKUP TREE");
    return -1;
  }
  // Test case 1: Insert key-value pairs
  KeyValue kv1 = {5, 3, stringToCharArray("apple"), stringToCharArray("red")};
  KeyValue kv2 = {6, 6, stringToCharArray("banana"),
                  stringToCharArray("yellow")};
  KeyValue kv3 = {6, 3, stringToCharArray("cherry"), stringToCharArray("red")};
  KeyValue kv4 = {5, 6, stringToCharArray("grape"),
                  stringToCharArray("purple")};
  KeyValue kv5 = {4, 5, stringToCharArray("pear"), stringToCharArray("green")};

  insert(tree, kv1);
  insert(tree, kv2);
  insert(tree, kv3);
  insert(tree, kv4);
  insert(tree, kv5);

  // Test case 2: Retrieve inserted keys
  KeyValue result1, result2, result3, result4, result5;

  // Test case 1: Insert key-value pairs
  int found1 = searchKeyValue(tree, "apple", &result1);
  int found2 = searchKeyValue(tree, "banana", &result2);
  int found3 = searchKeyValue(tree, "cherry", &result3);
  int found4 = searchKeyValue(tree, "grape", &result4);
  int found5 = searchKeyValue(tree, "pear", &result5);

  // Assert the results
  assert(found1 &&
         strcmp(charArrayToString(result1.value, result1.vlen), "red") == 0);
  assert(found2 &&
         strcmp(charArrayToString(result2.value, result2.vlen), "yellow") == 0);
  assert(found3 &&
         strcmp(charArrayToString(result3.value, result3.vlen), "red") == 0);
  assert(found4 &&
         strcmp(charArrayToString(result4.value, result4.vlen), "purple") == 0);
  assert(found5 &&
         strcmp(charArrayToString(result5.value, result5.vlen), "green") == 0);

  printf("All test cases passed!\n");

  return 0;
}
