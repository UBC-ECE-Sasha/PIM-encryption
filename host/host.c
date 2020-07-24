#include "common.h"
#include "pim_crypto.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TESTDATA_FOREACH_BLOCK(block_var, index_var)                           \
  for (unsigned long long block_var = 0, index_var = 0;                        \
       block_var < (test_data_size / sizeof(unsigned long long)) /             \
                       (16 / sizeof(unsigned long long));                      \
       block_var++, index_var += 16 / sizeof(unsigned long long))

#define USAGE "Usage: pimcrypto [data_length] [number_of_dpus]" \
  "\n\ndata_length may not be 0, and may use K, M, or G to indicate units.\n" \
  "number_of_dpus must be between 1 and the number of DPUs available on your system.\n"

int main(int argc, const char* argv[]) {

  unsigned long test_data_size;
  unsigned int nr_of_dpus;

  if (argc == 1) {
    test_data_size = 16 << 10; // 16KB - good for simulation
    nr_of_dpus = 1;
  } else if (argc == 3) {
    char * unit;
    test_data_size = strtol(argv[1], &unit, 0);

    if (test_data_size == 0) {
      ERROR(USAGE);
      return 1;
    }

    switch (*unit) {
      case '\0': break;
      case 'K': test_data_size <<= 10; break;
      case 'M': test_data_size <<= 20; break;
      case 'G': test_data_size <<= 30; break;
      default: ERROR(USAGE); return 1;
    }

    nr_of_dpus = atoi(argv[2]);

    if (nr_of_dpus == 0) {
      ERROR(USAGE);
      return 1;
    }
  } else {
    ERROR(USAGE);
    return 1;
  }

  DEBUG("Performing encryption with %lu bytes and %d DPUs\n\n", test_data_size, nr_of_dpus);

  unsigned long long * buffer = malloc(test_data_size);
  if (buffer == NULL) {
    ERROR("Could not allocate test data buffer.\n");
    exit(1);
  }

  TESTDATA_FOREACH_BLOCK(block, index) { buffer[index] = block; }

  unsigned char key[KEY_BUFFER_SIZE];
  memcpy(key, TEST_KEY, KEY_BUFFER_SIZE);

  if (dpu_AES_ecb(buffer, buffer, test_data_size, key, OP_ENCRYPT, nr_of_dpus) == -1) {
    ERROR("Encryption failed.\n");
  }

  TESTDATA_FOREACH_BLOCK(block, index) {
    if (buffer[index] == block) {
      ERROR(
          "Validation error: Buffer block %lld (index %lld) is not encrypted\n",
          block, index);
      return 1;
    }
  }

  if (dpu_AES_ecb(buffer, buffer, test_data_size, key, OP_DECRYPT, nr_of_dpus) == -1) {
    ERROR("Decryption failed.\n");
  }

  TESTDATA_FOREACH_BLOCK(block, index) {
    if (buffer[index] != block) {
      ERROR("Validation error: Buffer block %lld (index %lld) did not decrypt "
             "to its original value (block value is %lld%lld, should be 0%lld)\n",
             block, index, buffer[index - 1], buffer[index], block);
      return 1;
    }
  }

  free(buffer);
  return 0;
}
