#ifndef COMMON_H
#define COMMON_H

#include "PIM-common/common/include/common.h"

#define XSTR(x) STR(x)
#define STR(x) #x

#define DPU_BUFFER dpu_mram_buffer
#define DPU_BUFFER_SIZE MRAM_SIZE

#define KEY_BUFFER dpu_key_buffer
#define KEY_BUFFER_SIZE AES_KEY_SIZE_BYTES

#define DPU_DATA_SIZE dpu_data_size

#define AES_KEY_SIZE 128
#define AES_KEY_SIZE_BYTES (AES_KEY_SIZE / 8)

#define TEST_KEY "hello world hello world"

#define TRANSFER_SIZE 2048
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
