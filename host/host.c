#include "common.h"
#include "pim_crypto.h"
#include "tests.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TEST_STRING "Hello, world!"

static char buffer[DPU_BUFFER_SIZE];

int main() {

  // Add some test data to show that encryption is actually working
  strcpy(buffer, TEST_STRING);

  unsigned char host_buffer[1024 * 1024];
  unsigned char key[KEY_BUFFER_SIZE];
  memcpy(key, TEST_KEY, KEY_BUFFER_SIZE);

  host_encrypt(key, host_buffer);

  printf("Plaintext: %s\n", buffer);
  dpu_AES_ecb(buffer, buffer, DPU_BUFFER_SIZE, key, OP_ENCRYPT);

  printf("Ciphertext: %s\n", buffer);
  dpu_AES_ecb(buffer, buffer, DPU_BUFFER_SIZE, key, OP_DECRYPT);

  printf("Decrypted plaintext: %s\n", buffer);

  return 0;
}
