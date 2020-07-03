#include "common.h"
#include "pim_crypto.h"
#include "tests.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TESTDATA_FOREACH_BLOCK(block_var, index_var)                           \
  for (unsigned long long block_var, index_var = 0;                            \
       block_var < (DPU_BUFFER_SIZE / sizeof(unsigned long long)) /            \
                       (16 / sizeof(unsigned long long));                      \
       block_var++, index_var += 16 / sizeof(unsigned long long))

static unsigned long long buffer[DPU_BUFFER_SIZE / sizeof(unsigned long long)];

int main() {

  TESTDATA_FOREACH_BLOCK(block, index) { buffer[index] = block; }

  unsigned char host_buffer[1024 * 1024];
  unsigned char key[KEY_BUFFER_SIZE];
  memcpy(key, TEST_KEY, KEY_BUFFER_SIZE);

  host_encrypt(key, host_buffer);

  dpu_AES_ecb(buffer, buffer, DPU_BUFFER_SIZE, key, OP_ENCRYPT);

  TESTDATA_FOREACH_BLOCK(block, index) {
    if (buffer[index] == block) {
      printf(
          "Validation error: Buffer block %lld (index %lld) is not encrypted",
          block, index);
      return 1;
    }
  }

  dpu_AES_ecb(buffer, buffer, DPU_BUFFER_SIZE, key, OP_DECRYPT);

  TESTDATA_FOREACH_BLOCK(block, index) {
    if (buffer[index] != block) {
      printf("Validation error: Buffer block %lld (index %lld) did not decrypt "
             "to its original value (block value is %lld%lld, should be 0%lld)",
             block, index, buffer[index - 1], buffer[index], block);
      return 1;
    }
  }

  return 0;
}
