#include "aes_core.h"
#include "aes_local.h"
#include "common.h"
#include <mram.h>

#define TRANSFER_SIZE 2048
#define BLOCKS_PER_TRANSFER TRANSFER_SIZE / 16

__dma_aligned unsigned char buf[TRANSFER_SIZE];
__mram_noinit uint8_t DPU_BUFFER[DPU_BUFFER_SIZE];

int main(void) {
  unsigned char key_buf[] = "hello world hello world";
  AES_KEY key;

#ifndef DECRYPT
  AES_set_encrypt_key(key_buf, 128, &key);
#else
  AES_set_decrypt_key(key_buf, 128, &key);
#endif

  mram_read(DPU_BUFFER, buf, TRANSFER_SIZE);

  for (unsigned int block = 0; block < BLOCKS_PER_TRANSFER; block++) {
    unsigned char *block_ptr = buf + 16 * block;
#ifndef DECRYPT
    AES_encrypt(block_ptr, block_ptr, &key);
#else
    AES_decrypt(block_ptr, block_ptr, &key);
#endif
  }
  mram_write(buf, DPU_BUFFER, TRANSFER_SIZE);

  return 0;
}
