#include "common.h"
#include "tests.h"
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

  struct dpu_set_t dpu_set;
  uint32_t nr_of_dpus;

  DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
  DPU_ASSERT(dpu_load(dpu_set, DPU_ENCRYPT_BINARY, NULL));

  DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
  printf("Allocated %d DPU(s)\n", nr_of_dpus);

  DPU_ASSERT(
      dpu_copy_to(dpu_set, XSTR(DPU_BUFFER), 0, buffer, DPU_BUFFER_SIZE));
  DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

  struct dpu_set_t dpu;
  uint32_t dpu_perfcount;
  DPU_FOREACH(dpu_set, dpu) {
    DPU_ASSERT(
        dpu_copy_from(dpu, XSTR(DPU_BUFFER), 0, buffer, DPU_BUFFER_SIZE));
    dpu_copy_from(dpu, "dpu_perfcount", 0, &dpu_perfcount,
                  sizeof(dpu_perfcount));
    printf("Performance count: %d\n", dpu_perfcount);
  }

  DPU_ASSERT(dpu_free(dpu_set));
}
