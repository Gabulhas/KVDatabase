#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint16_t bytesToUInt16(unsigned char *byteArray, int startIndex) {
  uint16_t result =
      (uint16_t)byteArray[startIndex] << 8 | byteArray[startIndex + 1];
  return result;
}

uint64_t bytesToUInt64(unsigned char *byteArray, int startIndex) {
  uint64_t result = 0;

  for (int i = 0; i < 8; i++) {
    result = (result << 8) | byteArray[startIndex + i];
  }

  return result;
}

void uint16ToBytes(uint16_t value, unsigned char *byteArray, int startIndex) {
  byteArray[startIndex] = (value >> 8) & 0xFF;
  byteArray[startIndex + 1] = value & 0xFF;
}

void uint64ToBytes(uint64_t value, unsigned char *byteArray, int startIndex) {
  for (int i = 7; i >= 0; i--) {
    byteArray[startIndex + i] = value & 0xFF;
    value >>= 8;
  }
}

void uintToBytes(uintmax_t value, unsigned char *byteArray, int startIndex,
                 size_t numBytes) {
  for (size_t i = numBytes - 1; i < numBytes; i--) {
    byteArray[startIndex + i] = value & 0xFF;
    value >>= 8;
  }
}

char *charArrayToString(char *arr, uint16_t len) {
  char *result = malloc(sizeof(char) * (len + 1));
  memcpy(result, arr, len);
  result[len] = '\0';
  return result;
}

char *stringToCharArray(char *arr) {
  size_t stringLength = strlen(arr);

  char *charArray = (char *)malloc(sizeof(char) * stringLength);

  for (size_t i = 0; i < stringLength; i++) {
    charArray[i] = arr[i];
  }
  return charArray;
}
