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

int dpu_AES_ecb(const void *in, void *out, unsigned int length, const void *key,
                int operation) {

  if (operation != OP_ENCRYPT && operation != OP_DECRYPT) {
    printf("Invalid operation\n");
    return -1;
  }

  if (length % 128 != 0) {
    printf("Length is not a multiple of block size\n");
    return -1;
  }

  struct timeval dpu_start, dpu_end;

  struct dpu_set_t dpu_set;
  uint32_t nr_of_dpus;

#if NR_DPUS != 0
  DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
#else
  int nr_of_dpus_wanted = length / MIN_CHUNK_SIZE;
  dpu_error_t error = dpu_alloc(nr_of_dpus_wanted, NULL, &dpu_set);

  if (error == DPU_ERR_ALLOCATION) {
    DPU_ASSERT(dpu_alloc(DPU_ALLOCATE_ALL, NULL, &dpu_set));
  } else {
    DPU_ASSERT(error);
  }
#endif

  DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
  int chunk_size = length / nr_of_dpus;

  if (chunk_size > MRAM_SIZE) { // More data than will fit in MRAM
    printf("Data does not fit in MRAM (%d bytes into %d DPUs)\n", length, nr_of_dpus);
    DPU_ASSERT(dpu_free(dpu_set));
    return -1;
  }

  if (chunk_size % 128 != 0) { // Some blocks are not whole
    printf("Length is not a multiple of block size when split across %d DPUs\n", nr_of_dpus);
    DPU_ASSERT(dpu_free(dpu_set));
    return -1;
  }

  if (length % chunk_size != 0) { // Data does not fit evenly onto DPUs
    printf("%d bytes cannot be split evenly across %d DPUs\n", length, nr_of_dpus);
    DPU_ASSERT(dpu_free(dpu_set));
    return -1;
  }

  printf("Using %4.d DPU(s) %2.d tasklets, ", nr_of_dpus, NR_TASKLETS);

  if (operation == OP_ENCRYPT) {
    DPU_ASSERT(dpu_load(dpu_set, DPU_ENCRYPT_BINARY, NULL));
  } else {
    DPU_ASSERT(dpu_load(dpu_set, DPU_DECRYPT_BINARY, NULL));
  }

  uint64_t offset = 0;

  struct dpu_set_t dpu;
  DPU_FOREACH(dpu_set, dpu) {
    DPU_ASSERT(
        dpu_copy_to(dpu, XSTR(DPU_BUFFER), 0, in + offset, chunk_size));

    offset += chunk_size;
  }

  DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(KEY_BUFFER), 0, key, KEY_BUFFER_SIZE));

  gettimeofday(&dpu_start, NULL);
  DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
  gettimeofday(&dpu_end, NULL);

  printf("%s took %3.2fs ",
         (operation == OP_ENCRYPT) ? "encryption" : "decryption",
         (double)(dpu_end.tv_sec - dpu_start.tv_sec) +
             (dpu_end.tv_usec - dpu_start.tv_usec) / 10E6);

  uint64_t dpu_perfcount;
  offset = 0;

  DPU_FOREACH(dpu_set, dpu) {
    DPU_ASSERT(
        dpu_copy_from(dpu, XSTR(DPU_BUFFER), 0, out + offset, chunk_size));

    dpu_copy_from(dpu, "dpu_perfcount", 0, &dpu_perfcount,
                  sizeof(dpu_perfcount));
    printf("%10.ld cycles\n", dpu_perfcount);

    offset += chunk_size;
  }

  DPU_ASSERT(dpu_free(dpu_set));
  return 0;
}
