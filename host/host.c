#include <assert.h>
#include <dpu.h>
#include <stdio.h>

#ifndef DPU_BINARY
#define DPU_BINARY "encrypt"
#endif

#define BUFFER_SIZE 64 * 1024 * 1024 // 64M

char buffer[BUFFER_SIZE];

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

  if (input_size != BUFFER_SIZE) {
    printf("Error: input file's size is not equal to the buffer size\n");
    fclose(plaintext_fp);
    fclose(ciphertext_fp);
    exit(1);
  }
  rewind(plaintext_fp);

  size_t bytes_read = fread(buffer, sizeof(char), BUFFER_SIZE,
                            plaintext_fp); // No size checking because we are
                                           // only supporting 64M files
  size_t bytes_written =
      fwrite(buffer, sizeof(char), BUFFER_SIZE, ciphertext_fp);

  fclose(plaintext_fp);
  fclose(ciphertext_fp);

  if (bytes_read != input_size || bytes_written != input_size) {
    printf("Something went wrong, filesizes are off!\nInput: %ld\tRead: "
           "%ld\tWritten: %ld\n",
           input_size, bytes_read, bytes_written);
    exit(1);
  }
}
