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

#if MRAM_SIZE % TRANSFER_SIZE != 0
#error "Transfer size is misaligned with MRAM size"
#endif

#define NR_BUFFERS 2
#define NEXT_BUFFER(current_buffer) ((current_buffer + 1) % NR_BUFFERS)
#define PREV_BUFFER(current_buffer) ((current_buffer - 1) % NR_BUFFERS)

#define NR_CRYPTO_TASKLETS (NR_TASKLETS - 1)
#define CRYPTO_START_OFFSET                                                    \
  ((me() - 1) * AES_BLOCK_SIZE_BYTES) // crypto tasklet 1 starts on block 0, crypto tasklet 2
                    // starts on block 1...
#define CRYPTO_INTERVAL (NR_CRYPTO_TASKLETS * AES_BLOCK_SIZE_BYTES)

__mram_noinit uint8_t DPU_DATA_BUFFER[DPU_DATA_BUFFER_SIZE];
__host perfcounter_t dpu_perfcount;

__host unsigned int DPU_LENGTH_BUFFER;
__host unsigned char DPU_KEY_BUFFER[DPU_KEY_BUFFER_SIZE];
AES_KEY key;

BARRIER_INIT(buffer_barrier, NR_TASKLETS);
struct transfer_unit transfer_buffers[NR_BUFFERS];
bool done = 0;

int do_dma(void) {
  unsigned int current_buffer = 0;
  __mram_ptr void *current_location = DPU_DATA_BUFFER;
  __mram_ptr void *end_location = DPU_DATA_BUFFER + DPU_LENGTH_BUFFER;

  transfer_buffers[current_buffer].src = current_location;
  read_transfer_unit(&transfer_buffers[current_buffer]);
  barrier_wait(&buffer_barrier); // current: encrypting, next: garbage

  while (current_location + TRANSFER_SIZE < (__mram_ptr void *)MRAM_SIZE &&
      current_location < end_location) { // end when next doesn't exist, or we've encrypted all data
    transfer_buffers[NEXT_BUFFER(current_buffer)].src = current_location + TRANSFER_SIZE;
    read_transfer_unit(&transfer_buffers[NEXT_BUFFER(current_buffer)]);

    barrier_wait(&buffer_barrier); // current: encrypted, next: encrypting
    write_transfer_unit(&transfer_buffers[current_buffer]);
    current_buffer = NEXT_BUFFER(current_buffer);
    current_location += TRANSFER_SIZE;
  }

  done = 1;
  barrier_wait(&buffer_barrier); // current: encrypted, next: doesn't exist
  write_transfer_unit(&transfer_buffers[current_buffer]);

  return 0;
}

int do_crypto(void) {

  int current_buffer = 0;

  while (1) {
    barrier_wait(&buffer_barrier);
    if (done) {
      return 0;
    }

    uint8_t *start_block = transfer_buffers[current_buffer].data;
    for (uint8_t *block_ptr = start_block + CRYPTO_START_OFFSET;
         block_ptr < start_block + TRANSFER_SIZE;
         block_ptr += CRYPTO_INTERVAL) {
#ifndef DECRYPT
      AES_encrypt(block_ptr, block_ptr, &key);
#else
      AES_decrypt(block_ptr, block_ptr, &key);
#endif
    }

    current_buffer = NEXT_BUFFER(current_buffer);
  }

  // Should never reach here
  return -1;
}

int main(void) {

  if (me() == 0) {
    perfcounter_config(PERFCOUNT_TYPE, true);
    int status = do_dma();
    dpu_perfcount = perfcounter_get();
    return status;
  } else {
    if (me() == 1) {
#ifndef DECRYPT
      AES_set_encrypt_key(DPU_KEY_BUFFER, AES_KEY_SIZE, &key);
#else
      AES_set_decrypt_key(DPU_KEY_BUFFER, AES_KEY_SIZE, &key);
#endif
    }
    return do_crypto();
  }
}
