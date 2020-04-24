#include "common.h"
#include <assert.h>
#include <dpu.h>
#include <stdio.h>

#ifndef DPU_ENCRYPT_BINARY
#define DPU_ENCRYPT_BINARY "build/dpu_encrypt"
#endif

#ifndef DPU_DECRYPT_BINARY
#define DPU_DECRYPT_BINARY "build/dpu_decrypt"
#endif

static char buffer[DPU_BUFFER_SIZE];

int main(int argc, char **argv) {
  if (argc < 4) {
    printf("Error: not enough arguments\nUsage:\n\t%s <encrypt|decrypt> "
           "<input> <output>\n",
           argv[0]);
    exit(1);
  }

  char *dpu_binary;
  if (strcmp(argv[1], "encrypt") == 0) {
    dpu_binary = DPU_ENCRYPT_BINARY;
  } else if (strcmp(argv[1], "decrypt") == 0) {
    dpu_binary = DPU_DECRYPT_BINARY;
  } else {
    printf("Error: invalid operation '%s'. Must be one of: encrypt, decrypt\n",
           argv[1]);
    exit(1);
  }

  FILE *input_fp;
  FILE *output_fp;

  input_fp = fopen(argv[2], "rb");
  output_fp = fopen(argv[3], "wb");

  fseek(input_fp, 0L, SEEK_END);
  size_t input_size = ftell(input_fp);

  if (input_size != DPU_BUFFER_SIZE) {
    printf("Error: input file's size is not equal to the buffer size\n");
    fclose(input_fp);
    fclose(output_fp);
    exit(1);
  }
  rewind(input_fp);

  size_t bytes_read = fread(buffer, sizeof(char), DPU_BUFFER_SIZE,
                            input_fp); // No size checking because we are
                                       // only supporting 64M files

  struct dpu_set_t dpu_set;
  uint32_t nr_of_dpus;

  DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
  DPU_ASSERT(dpu_load(dpu_set, dpu_binary, NULL));

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

  size_t bytes_written =
      fwrite(buffer, sizeof(char), DPU_BUFFER_SIZE, output_fp);

  fclose(input_fp);
  fclose(output_fp);

  if (bytes_read != input_size || bytes_written != input_size) {
    printf("Something went wrong, filesizes are off!\nInput: %ld\tRead: "
           "%ld\tWritten: %ld\n",
           input_size, bytes_read, bytes_written);
    exit(1);
  }
}
