#include "common.h"
#include "tests.h"
#include <sys/time.h>
#include <assert.h>
#include <dpu.h>
#include <stdio.h>

#ifndef DPU_ENCRYPT_BINARY
#define DPU_ENCRYPT_BINARY "build/dpu_encrypt"
#endif

#ifndef DPU_DECRYPT_BINARY
#define DPU_DECRYPT_BINARY "build/dpu_decrypt"
#endif

#define TEST_STRING "Hello, world!"

static char buffer[DPU_BUFFER_SIZE];

int main() {

  // Add some test data to show that encryption is actually working
  strcpy(buffer, TEST_STRING);

  unsigned char host_buffer[1024 * 1024];
  unsigned char key[16];
  memcpy(key, TEST_KEY, 16);

  host_encrypt(key, host_buffer);

  struct timeval dpu_start, dpu_end;

  struct dpu_set_t dpu_set;
  uint32_t nr_of_dpus;

  DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
  DPU_ASSERT(dpu_load(dpu_set, DPU_ENCRYPT_BINARY, NULL));

  DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
  printf("Using %4.d DPU(s) %2.d tasklets, ", nr_of_dpus, NR_TASKLETS);
  //printf("Plaintext : %s\n", buffer);

  DPU_ASSERT(
      dpu_copy_to(dpu_set, XSTR(DPU_BUFFER), 0, buffer, DPU_BUFFER_SIZE));

  gettimeofday(&dpu_start, NULL);
  DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
  gettimeofday(&dpu_end, NULL);

  printf("encryption took %3.2fs ", (double)(dpu_end.tv_sec - dpu_start.tv_sec) + (dpu_end.tv_usec - dpu_start.tv_usec) / 10E6);

  struct dpu_set_t dpu;
  uint32_t dpu_perfcount;
  DPU_FOREACH(dpu_set, dpu) {
    DPU_ASSERT(
        dpu_copy_from(dpu, XSTR(DPU_BUFFER), 0, buffer, DPU_BUFFER_SIZE));
    dpu_copy_from(dpu, "dpu_perfcount", 0, &dpu_perfcount,
                  sizeof(dpu_perfcount));
    //printf("Ciphertext: %s\n", buffer);
    printf("%10.d cycles\n", dpu_perfcount);
  }

  DPU_ASSERT(dpu_load(dpu_set, DPU_DECRYPT_BINARY, NULL));
  DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

  DPU_FOREACH(dpu_set, dpu) {
    DPU_ASSERT(
        dpu_copy_from(dpu, XSTR(DPU_BUFFER), 0, buffer, DPU_BUFFER_SIZE));
    dpu_copy_from(dpu, "dpu_perfcount", 0, &dpu_perfcount,
                  sizeof(dpu_perfcount));
    //printf("Plaintext : %s\n", buffer);
    printf("Performance count: %d\n", dpu_perfcount);
  }

  DPU_ASSERT(dpu_free(dpu_set));
}
