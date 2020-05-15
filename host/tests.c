#include "aes_core.h"
#include "aes_local.h"
#include "common.h"
#include <stdio.h>

void host_encrypt(unsigned char *key, unsigned char *data) {
  AES_KEY aes_key;
  AES_set_encrypt_key(key, 128, &aes_key);

  unsigned char *block_ptr = data;
  for (unsigned int block = 0; block < BLOCKS_PER_TRANSFER; block++) {
    AES_encrypt(block_ptr, block_ptr, &aes_key);
  }
}
