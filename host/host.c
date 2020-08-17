#include "PIM-common/common/include/common.h"
#include "common.h"
#include "crypto.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PIM_MODE 1
#define HOST_MODE 2

#define TEST_KEY "hello world hello world"

#define USAGE                                                                  \
  "usage: %s\n"                                                                \
  "       %s dpu <number_of_dpus> <encrypt|decrypt> <data_length>\n"           \
  "       %s host <encrypt|decrypt> <data_length>\n"                           \
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
  "\thost\tPerform encryption on the CPU\n"

#define PRINT_USAGE() ERROR(USAGE, argv[0], argv[0], argv[0])

int main(int argc, const char *argv[]) {

  unsigned long test_data_size;
  // Assigned to suppress a "may be uninitialized" warning
  unsigned int nr_of_dpus = 0;
  int mode;
  int operation;
  const char **mandatory_args;

  // Parse the mode
  if (argc == 1) {
    test_data_size = KILOBYTE(16); // good for simulation
    nr_of_dpus = 1;
    mode = PIM_MODE;
    operation = OP_ENCRYPT;
    DEBUG("Performing encryption with %lu bytes and %d DPUs\n\n",
          test_data_size, nr_of_dpus);
  } else if (strcmp(argv[1], "dpu") == 0) {
    if (argc != 5) {
      PRINT_USAGE();
      return 1;
    }
    mandatory_args = argv + 3;
    mode = PIM_MODE;

    nr_of_dpus = atoi(argv[2]);

    if (nr_of_dpus == 0) {
      PRINT_USAGE();
      return 1;
    }
  } else if (strcmp(argv[1], "host") == 0) {
    if (argc != 4) {
      PRINT_USAGE();
      return 1;
    }
    mandatory_args = argv + 2;
    mode = HOST_MODE;
  } else {
    PRINT_USAGE();
    return 1;
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
      DEBUG("Performing %sion on the CPU with %lu bytes\n\n",
            (argc == 1) ? "encrypt" : mandatory_args[0], test_data_size);
    }
  }

  unsigned long long *buffer = malloc(test_data_size);
  if (buffer == NULL) {
    ERROR("Could not allocate test data buffer.\n");
    exit(1);
  }

  unsigned char key[DPU_KEY_BUFFER_SIZE];
  memcpy(key, TEST_KEY, DPU_KEY_BUFFER_SIZE);

  int error;

  switch (mode) {
  case PIM_MODE:
    error =
        dpu_AES_ecb(buffer, buffer, test_data_size, key, operation, nr_of_dpus);
    break;
  case HOST_MODE:
    error = host_AES_ecb(buffer, buffer, test_data_size, key, operation);
    break;
  default:
    ERROR("Unknown mode! This should be an unreachable state!");
    exit(1);
  }

  if (error == -1) {
    ERROR("Operation failed.\n");
    exit(1);
  }

  free(buffer);
  return 0;
}
