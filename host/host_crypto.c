#include "PIM-common/host/include/host.h"
#include "aes_core.h"
#include "aes_local.h"
#include "common.h"
#include "crypto.h"
#include <stdio.h>
#include <time.h>

int host_AES_ecb(void *in, void *out, unsigned long length, const void *key_ptr,
                 int operation) {

  struct timespec start, end;

  clock_gettime(CLOCK_MONOTONIC_RAW, &start);

  switch (operation) {
  case OP_ENCRYPT:
    host_AES_ecb_encrypt(in, out, length, key_ptr);
    break;
  case OP_DECRYPT:
    host_AES_ecb_decrypt(in, out, length, key_ptr);
    break;
  default:
    return 1;
  }

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);

  double execution_time = TIME_DIFFERENCE(start, end);

  // TODO: add a cycle count
  // Operation, Data size, Execution time
  MEASURE("%d,%ld,%f\n", operation, length, execution_time);

  DEBUG("%sed %ld bytes in %fs\n",
        (operation == OP_ENCRYPT) ? "Encrypt" : "Decrypt", length,
        execution_time);

  return 0;
}

int host_AES_ecb_encrypt(void *in, void *out, unsigned long length,
                         const void *key_ptr) {
  AES_KEY key;
  AES_set_encrypt_key(key_ptr, AES_KEY_SIZE, &key);

  for (unsigned long location = 0; location < length / 16; location++) {
    AES_encrypt(in + location * 16, out + location * 16, &key);
  }

  return 0;
}

int host_AES_ecb_decrypt(void *in, void *out, unsigned long length,
                         const void *key_ptr) {
  AES_KEY key;
  AES_set_decrypt_key(key_ptr, AES_KEY_SIZE, &key);

  for (unsigned long location = 0; location < length / 16; location++) {
    AES_decrypt(in + location * 16, out + location * 16, &key);
  }

  return 0;
}
