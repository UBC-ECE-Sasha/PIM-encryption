#include "aes_core.c"
#include "aes_local.h"
#include <mram.h>

#define MRAM_SIZE 0x0
#define TRANSFER_SIZE 2048
#define BLOCKS_PER_TRANSFER TRANSFER_SIZE / 16

__dma_aligned unsigned char buf[TRANSFER_SIZE];

int main(void) {
  unsigned char key[] = "hello world hello world";
  AES_KEY encrypt_key;
  AES_set_encrypt_key(key, 128, &encrypt_key);

  __mram_ptr unsigned char *mram_ptr = 0x0;

  mram_read(mram_ptr, buf, TRANSFER_SIZE);

  for (unsigned int block = 0; block < BLOCKS_PER_TRANSFER; block++) {
    unsigned char *block_ptr = buf + 16 * block;
    AES_encrypt(block_ptr, block_ptr, &encrypt_key);
  }
  mram_write(buf, mram_ptr, TRANSFER_SIZE);

  return 0;
}
