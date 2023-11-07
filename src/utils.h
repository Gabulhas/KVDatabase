#include <stdint.h>
#include <stdlib.h>
uint16_t bytesToUInt16(unsigned char *byteArray, int startIndex);
uint64_t bytesToUInt64(unsigned char *byteArray, int startIndex);
void uint16ToBytes(uint16_t value, unsigned char *byteArray, int startIndex);
void uint64ToBytes(uint64_t value, unsigned char *byteArray, int startIndex);
void uintToBytes(uintmax_t value, unsigned char *byteArray, int startIndex,
                 size_t numBytes);
char *charArrayToString(char *arr, uint16_t len);
char *stringToCharArray(char *arr);
