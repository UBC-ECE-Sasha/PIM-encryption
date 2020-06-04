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

int dpu_AES_ecb(const void *in, void *out, int length, const void *key,
                int operation) {

  if (operation != OP_ENCRYPT && operation != OP_DECRYPT) {
    return -1;
  }

  if (length % 128 != 0) {
    return -1;
  }

  if (length > MRAM_SIZE) {
    return -1;
  }

  struct timeval dpu_start, dpu_end;

  struct dpu_set_t dpu_set;
  uint32_t nr_of_dpus;

  DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));

  if (operation == OP_ENCRYPT) {
    DPU_ASSERT(dpu_load(dpu_set, DPU_ENCRYPT_BINARY, NULL));
  } else {
    DPU_ASSERT(dpu_load(dpu_set, DPU_DECRYPT_BINARY, NULL));
  }

  DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(KEY_BUFFER), 0, key, KEY_BUFFER_SIZE));

  DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
  printf("Using %4.d DPU(s) %2.d tasklets, ", nr_of_dpus, NR_TASKLETS);

  DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(DPU_BUFFER), 0, in, length));

  gettimeofday(&dpu_start, NULL);
  DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
  gettimeofday(&dpu_end, NULL);

  printf("%s took %3.2fs ",
         (operation == OP_ENCRYPT) ? "encryption" : "decryption",
         (double)(dpu_end.tv_sec - dpu_start.tv_sec) +
             (dpu_end.tv_usec - dpu_start.tv_usec) / 10E6);

  struct dpu_set_t dpu;
  uint64_t dpu_perfcount;
  DPU_FOREACH(dpu_set, dpu) {
    DPU_ASSERT(dpu_copy_from(dpu, XSTR(DPU_BUFFER), 0, out, length));
    dpu_copy_from(dpu, "dpu_perfcount", 0, &dpu_perfcount,
                  sizeof(dpu_perfcount));
    printf("%10.ld cycles\n", dpu_perfcount);
  }

  DPU_ASSERT(dpu_free(dpu_set));
  return 0;
}
