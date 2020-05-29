#include "aes_core.h"
#include "aes_local.h"
#include "common.h"
#include "transfer_unit.h"
#include <barrier.h>
#include <defs.h>
#include <mram.h>
#include <perfcounter.h>

#if NR_TASKLETS < 2
#error "Not enough tasklets - requires at least two"
#endif

#define NR_BUFFERS 2
#define NR_CRYPTO_TASKLETS NR_TASKLETS - 1

__mram_noinit uint8_t DPU_BUFFER[DPU_BUFFER_SIZE];
__host uint32_t dpu_perfcount;

unsigned char key_buf[] = TEST_KEY;
AES_KEY key;

BARRIER_INIT(buffer_barrier, NR_TASKLETS);
struct transfer_unit transfer_buffers[NR_BUFFERS];
bool done = 0;

int do_dma(void) {
  read_transfer_unit(DPU_BUFFER, &transfer_buffers[0]);
  barrier_wait(&buffer_barrier);

  __mram_ptr void *current_location = DPU_BUFFER + TRANSFER_SIZE;
  unsigned int current_buffer = 1;

  // while (current_location < (__mram_ptr void *)MRAM_SIZE) {
  while (current_location < (__mram_ptr void *)2048) {
    read_transfer_unit(current_location, &transfer_buffers[current_buffer]);
    current_location += TRANSFER_SIZE;
    current_buffer = (current_buffer + 1) % NR_BUFFERS;

    barrier_wait(&buffer_barrier);
    write_transfer_unit(&transfer_buffers[current_buffer]);
  }

  done = 1;
  barrier_wait(&buffer_barrier);
  unsigned int previous_buffer = (current_buffer - 1) % NR_BUFFERS;
  write_transfer_unit(&transfer_buffers[previous_buffer]);

  return 1;
}

int do_crypto(void) {

  int current_buffer = 0;

  while (1) {
    barrier_wait(&buffer_barrier);
    if (done) {
      return 0;
    }

    void *start_block = transfer_buffers[current_buffer].data;
    for (void *block_ptr = start_block; block_ptr < start_block + TRANSFER_SIZE;
         block_ptr += 16) {
#ifndef DECRYPT
      AES_encrypt(block_ptr, block_ptr, &key);
#else
      AES_decrypt(block_ptr, block_ptr, &key);
#endif
    }
  }

  // Should never reach here
  return -1;
}

int main(void) {

  if (me() == 0) {
    return do_dma();
  } else {
    if (me() == 1) {
#ifndef DECRYPT
      AES_set_encrypt_key(key_buf, 128, &key);
#else
      AES_set_decrypt_key(key_buf, 128, &key);
#endif
    }
    return do_crypto();
  }
}
