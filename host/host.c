#include "PIM-common/common/include/common.h"
#include "common.h"
#include "crypto.h"
#include <assert.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PIM_MODE 1
#define HOST_MODE 2
#define AESNI_MODE 3

#define TEST_KEY "hello world hello world"

#define USAGE                                                                  \
  "usage: %s\n"                                                                \
  "       %s dpu <number_of_dpus> <encrypt|decrypt> <data_length>\n"           \
  "       %s host <encrypt|decrypt> <data_length>\n"                           \
  "       %s aesni <encrypt|decrypt> <data_length>\n"                          \
  "Encrypt or decrypt a buffer of the specified size and output performance "  \
  "information.\n"                                                             \
  "By default, 16KB is encrypted using one DPU.\n\n"                           \
  ""                                                                           \
  "number_of_dpus:\tNumber of DPUs to spread data across. Must be larger "     \
  "than 0.\n"                                                                  \
  "data_length:\tAmount of data to use. Defaults to bytes, but may use K, M, " \
  "or G instead.\n\n"                                                          \
  ""                                                                           \
  "Available modes:\n"                                                         \
  "\tdpu\tPerform encryption in memory, using the specified number of DPUs\n"  \
  "\thost\tPerform encryption on the CPU\n"                                    \
  "\taesni\tPerform encryption on the CPU using AES-NI hardware"               \
  " acceleration\n"

#define PRINT_USAGE() ERROR(USAGE, argv[0], argv[0], argv[0], argv[0])

int main(int argc, const char *argv[]) {

  unsigned long test_data_size;
  // Assigned to suppress a "may be uninitialized" warning
  unsigned int nr_of_dpus = 0;
  int mode;
  int operation;
  const char **mandatory_args;

  // Parse the mode

  // Default - no arguments
  if (argc == 1) {
    test_data_size = KILOBYTE(16); // good for simulation
    nr_of_dpus = 1;
    mode = PIM_MODE;
    operation = OP_ENCRYPT;
    DEBUG("Performing encryption with %lu bytes and %d DPUs\n\n",
          test_data_size, nr_of_dpus);
  } else {
    if (strcmp(argv[1], "dpu") == 0) {
      mode = PIM_MODE;
    } else if (strcmp(argv[1], "host") == 0) {
      mode = HOST_MODE;
    } else if (strcmp(argv[1], "aesni") == 0) {
      mode = AESNI_MODE;
    } else {
      PRINT_USAGE();
      exit(1);
    }

    if (mode == PIM_MODE) {
      // PIM mode
      if (argc != 5) {
        PRINT_USAGE();
        return 1;
      }
      mandatory_args = argv + 3;

      nr_of_dpus = atoi(argv[2]);

      if (nr_of_dpus == 0) {
        PRINT_USAGE();
        return 1;
      }
    } else {
      // Host and AES-NI modes
      if (argc != 4) {
        PRINT_USAGE();
        return 1;
      }
      mandatory_args = argv + 2;
    }
  }

  if (argc != 1) {
    // Parse <encrypt|decrypt>
    if (strcmp(mandatory_args[0], "encrypt") == 0) {
      operation = OP_ENCRYPT;
    } else if (strcmp(mandatory_args[0], "decrypt") == 0) {
      operation = OP_DECRYPT;
    } else {
      PRINT_USAGE();
      return 1;
    }

    // Parse <data_length>
    char *unit;
    test_data_size = strtol(mandatory_args[1], &unit, 0);

    if (test_data_size == 0) {
      PRINT_USAGE();
      return 1;
    }

    switch (*unit) {
    case '\0':
      break; // default: bytes
    case 'K':
      test_data_size = KILOBYTE(test_data_size);
      break;
    case 'M':
      test_data_size = MEGABYTE(test_data_size);
      break;
    case 'G':
      test_data_size = GIGABYTE(test_data_size);
      break;
    default:
      PRINT_USAGE();
      return 1;
    }

    if (mode == PIM_MODE) {
      DEBUG("Performing %sion with %lu bytes and %d DPUs\n\n",
            (argc == 1) ? "encrypt" : mandatory_args[0], test_data_size,
            nr_of_dpus);
    } else {
      DEBUG("Performing %sion in %s mode with %lu bytes\n\n", mandatory_args[0],
            argv[1], test_data_size);
    }
  }

  unsigned long long *buffer, *out = NULL; // fix -Werror=maybe-uninitialized
  if (mode == AESNI_MODE && test_data_size > INT_MAX) {
    // We can't do an AES-NI operation this large all at once, so we break
    // it into 1GB chunks and re-use most of the same buffer
    //
    // This also makes it possible to do 32GB AES-NI operations on
    // upmemcloud1, which can't fit two 32GB buffers because it only has
    // 64GB of non-PIM memory
    buffer = malloc(test_data_size + GIGABYTE(1));
  } else {
    buffer = malloc(test_data_size);
  }

  // OpenSSL doesn't allow overlapping in and out buffers
  if (mode == AESNI_MODE && test_data_size <= INT_MAX) {
    out = malloc(test_data_size);
    if (out == NULL) {
      ERROR("Could not allocate output buffer.\n");
      return 1;
    }
  }

  if (buffer == NULL) {
    ERROR("Could not allocate test data buffer.\n");
    exit(1);
  }

  unsigned char key[DPU_KEY_BUFFER_SIZE];
  memcpy(key, TEST_KEY, DPU_KEY_BUFFER_SIZE);

  int error = 0;

  switch (mode) {
  case PIM_MODE:
    error =
        dpu_AES_ecb(buffer, buffer, test_data_size, key, operation, nr_of_dpus);
    break;
  case HOST_MODE:
    error = host_AES_ecb(buffer, buffer, test_data_size, key, operation);
    break;
  case AESNI_MODE:
    if (test_data_size <= INT_MAX) {
      error = aesni_AES_ecb(buffer, out, test_data_size, key, operation);
    } else {
      int chunk_size = GIGABYTE(1) / sizeof(unsigned long long);
      unsigned long long *buffer_end =
          buffer + (test_data_size + GIGABYTE(1)) / sizeof(unsigned long long);
      unsigned long long *inp = buffer + chunk_size, *outp = buffer;

      while (inp + chunk_size <= buffer_end) {
        aesni_AES_ecb(inp, outp, GIGABYTE(1), key, operation);
        inp += chunk_size;
        outp += chunk_size;
      }

      unsigned long long extra_data = test_data_size % GIGABYTE(1);
      if (extra_data != 0) {
        aesni_AES_ecb(inp, outp, extra_data, key, operation);
      }
    }
    break;
  default:
    ERROR("Unknown mode! This should be an unreachable state!");
    exit(1);
  }

  if (error != 0) {
    ERROR("Operation failed (errno %d)\n", error);
    return 1;
  }

  free(buffer);

  if (mode == AESNI_MODE && test_data_size <= INT_MAX) {
    free(out);
  }

  return 0;
}
