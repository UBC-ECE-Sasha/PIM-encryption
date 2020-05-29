#ifndef COMMON_H
#define COMMON_H

#define XSTR(x) STR(x)
#define STR(x) #x

#define DPU_BUFFER dpu_mram_buffer
#define DPU_BUFFER_SIZE 0x4000000

#define TEST_KEY "hello world hello world"

#define TRANSFER_SIZE 2048
#define BLOCKS_PER_TRANSFER TRANSFER_SIZE / 16

#define MRAM_SIZE (64 << 20)

#endif /* !COMMON_H */
