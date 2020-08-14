#ifndef COMMON_H
#define COMMON_H

#include "PIM-common/common/include/common.h"

#define XSTR(x) STR(x)
#define STR(x) #x

// Symbols configured by the makefile:
// NR_TASKLETS    Number of tasklets
// BULK           If defined, use push_xfer instead of dpu_copy_to
// TRANSFER_SIZE  Size of transfers between MRAM and WRAM
// PERFCOUNT_TYPE COUNT_CYCLES or COUNT_INSTRUCTIONS

// Define symbols for copying parameters and data to the DPU
#define DPU_DATA_BUFFER dpu_mram
#define DPU_KEY_BUFFER dpu_key
#define DPU_LENGTH_BUFFER dpu_length

#define DPU_KEY_BUFFER_SIZE AES_KEY_SIZE_BYTES
#define DPU_DATA_BUFFER_SIZE MRAM_SIZE

#define AES_BLOCK_SIZE 128
#define AES_BLOCK_SIZE_BYTES (AES_BLOCK_SIZE / 8)

#define AES_KEY_SIZE 128
#define AES_KEY_SIZE_BYTES (AES_KEY_SIZE / 8)

// TRANSFER_SIZE should be set by the makefile
#define BLOCKS_PER_TRANSFER TRANSFER_SIZE / 16

#ifndef EXPERIMENT
#  define DEBUG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#  define MEASURE(fmt, ...)
#else
#  define DEBUG(fmt, ...)
#  define MEASURE(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

#define ERROR(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)


#endif /* !COMMON_H */
