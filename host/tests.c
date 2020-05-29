#include "aes_core.h"
#include "aes_local.h"
#include "common.h"
#include <stdint.h>
#include <stdio.h>
#include <time.h>

__inline__ uint64_t rdtsc() {
  uint64_t a, d;
  __asm__ volatile("rdtsc" : "=a"(a), "=d"(d));
  return (d << 32) | a;
}

void host_encrypt(unsigned char *key, unsigned char *data) {
  AES_KEY aes_key;
  AES_set_encrypt_key(key, 128, &aes_key);

  unsigned char *block_ptr = data;
  uint64_t cycles_low, cycles_delta;

  for (int i = 0; i < 10000; i++) {
    for (unsigned int block = 0; block < BLOCKS_PER_TRANSFER; block++) {
      cycles_low = rdtsc();
      AES_encrypt(block_ptr, block_ptr, &aes_key);
      cycles_delta = rdtsc() - cycles_low;
    }
  }

  printf("Host cycles/block: %ld\n", cycles_delta);
}
