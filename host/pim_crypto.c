#include "pim_crypto.h"
#include "common.h"
#include <dpu.h>
#include <stdio.h>
#include <sys/time.h>

#ifndef DPU_ENCRYPT_BINARY
#define DPU_ENCRYPT_BINARY "build/dpu_encrypt"
#endif

#ifndef DPU_DECRYPT_BINARY
#define DPU_DECRYPT_BINARY "build/dpu_decrypt"
#endif

int dpu_AES_ecb(void *in, void *out, unsigned long length, const void *key,
                int operation, unsigned int nr_of_dpus) {

  if (operation != OP_ENCRYPT && operation != OP_DECRYPT) {
    ERROR("Invalid operation\n");
    return -1;
  }

  if (length % 128 != 0) {
    ERROR("Length is not a multiple of block size\n");
    return -1;
  }

  struct timeval dpu_start, dpu_end;

  struct dpu_set_t dpu_set;
  uint32_t real_nr_dpus;

  if (nr_of_dpus != 0) {
    DPU_ASSERT(dpu_alloc(nr_of_dpus, NULL, &dpu_set));
  } else {
    int nr_of_dpus_wanted = length / MIN_CHUNK_SIZE;
    dpu_error_t error = dpu_alloc(nr_of_dpus_wanted, NULL, &dpu_set);

    if (error == DPU_ERR_ALLOCATION) {
      DPU_ASSERT(dpu_alloc(DPU_ALLOCATE_ALL, NULL, &dpu_set));
    } else {
      DPU_ASSERT(error);
    }
  }

  DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &real_nr_dpus));
  int chunk_size = length / real_nr_dpus;

  if (chunk_size > MRAM_SIZE) { // More data than will fit in MRAM
    ERROR("Data does not fit in MRAM (%ld bytes into %d DPUs)\n", length, real_nr_dpus);
    DPU_ASSERT(dpu_free(dpu_set));
    return -1;
  }

  if (chunk_size % 128 != 0) { // Some blocks are not whole
    ERROR("Length is not a multiple of block size when split across %d DPUs\n", real_nr_dpus);
    DPU_ASSERT(dpu_free(dpu_set));
    return -1;
  }

  if (length % chunk_size != 0) { // Data does not fit evenly onto DPUs
    ERROR("%ld bytes cannot be split evenly across %d DPUs\n", length, real_nr_dpus);
    DPU_ASSERT(dpu_free(dpu_set));
    return -1;
  }

  DEBUG("Using %4.d DPU(s) %2.d tasklets, ", real_nr_dpus, NR_TASKLETS);

  if (operation == OP_ENCRYPT) {
    DPU_ASSERT(dpu_load(dpu_set, DPU_ENCRYPT_BINARY, NULL));
  } else {
    DPU_ASSERT(dpu_load(dpu_set, DPU_DECRYPT_BINARY, NULL));
  }

  uint64_t offset = 0;

  struct dpu_set_t dpu;
  DPU_FOREACH(dpu_set, dpu) {
    DPU_ASSERT(dpu_prepare_xfer(dpu, in + offset));

    offset += chunk_size;
  }

  dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, XSTR(DPU_BUFFER), 0, chunk_size, DPU_XFER_DEFAULT);

  DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(KEY_BUFFER), 0, key, KEY_BUFFER_SIZE));
  DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(DPU_DATA_SIZE), 0, &chunk_size, sizeof(chunk_size)));

  gettimeofday(&dpu_start, NULL);
  DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
  gettimeofday(&dpu_end, NULL);

  DEBUG("%s took %3.2fs ",
         (operation == OP_ENCRYPT) ? "encryption" : "decryption",
         (double)(dpu_end.tv_sec - dpu_start.tv_sec) +
             (dpu_end.tv_usec - dpu_start.tv_usec) / 10E6);

  uint64_t cycles_min = 0;
  uint64_t cycles_max = 0;
  uint64_t cycles_avg = 0;
  offset = 0;

  DPU_FOREACH(dpu_set, dpu) {
    DPU_ASSERT(dpu_prepare_xfer(dpu, out + offset));

    offset += chunk_size;

    uint64_t cycles;
    dpu_copy_from(dpu, "dpu_perfcount", 0, &cycles,
                  sizeof(cycles));

    cycles_min = (cycles_min == 0) ? cycles : cycles_min;
    cycles_min = (cycles_min < cycles) ? cycles_min : cycles;
    cycles_max = (cycles_max > cycles) ? cycles_max : cycles;
    cycles_avg += cycles;
  }

  cycles_avg /= real_nr_dpus;
  DEBUG("%10.ld cycles avg, %10.ld cycles min, %10.ld cycles max\n", cycles_avg, cycles_min, cycles_max);
  dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, XSTR(DPU_BUFFER), 0, chunk_size, DPU_XFER_DEFAULT);

  DPU_ASSERT(dpu_free(dpu_set));
  return 0;
}
