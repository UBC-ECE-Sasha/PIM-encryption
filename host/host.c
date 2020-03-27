#include <assert.h>
#include <dpu.h>
#include <stdio.h>

#ifndef DPU_BINARY
#define DPU_BINARY "encrypt"
#endif

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

  fclose(plaintext_fp);
  fclose(ciphertext_fp);
}
