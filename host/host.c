#include "common.h"
#include <assert.h>
#include <dpu.h>
#include <stdio.h>

#ifndef DPU_BINARY
#define DPU_BINARY "build/encryption_dpu"
#endif

static char buffer[DPU_BUFFER_SIZE];

int main(int argc, char **argv) {
  if (argc < 3) {
    printf(
        "Error: not enough arguments\nUsage:\n\t./encrypt [input] [output]\n");
    exit(1);
  }

  FILE *plaintext_fp;
  FILE *ciphertext_fp;

  plaintext_fp = fopen(argv[1], "rb");
  ciphertext_fp = fopen(argv[2], "wb");

  fseek(plaintext_fp, 0L, SEEK_END);
  size_t input_size = ftell(plaintext_fp);

  if (input_size != DPU_BUFFER_SIZE) {
    printf("Error: input file's size is not equal to the buffer size\n");
    fclose(plaintext_fp);
    fclose(ciphertext_fp);
    exit(1);
  }
  rewind(plaintext_fp);

  size_t bytes_read = fread(buffer, sizeof(char), DPU_BUFFER_SIZE,
                            plaintext_fp); // No size checking because we are
                                           // only supporting 64M files

  struct dpu_set_t dpu_set;
  uint32_t nr_of_dpus;

  DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
  DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY, NULL));

  DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
  printf("Allocated %d DPU(s)\n", nr_of_dpus);

  DPU_ASSERT(dpu_free(dpu_set));

  size_t bytes_written =
      fwrite(buffer, sizeof(char), DPU_BUFFER_SIZE, ciphertext_fp);

  fclose(plaintext_fp);
  fclose(ciphertext_fp);

  if (bytes_read != input_size || bytes_written != input_size) {
    printf("Something went wrong, filesizes are off!\nInput: %ld\tRead: "
           "%ld\tWritten: %ld\n",
           input_size, bytes_read, bytes_written);
    exit(1);
  }
}
