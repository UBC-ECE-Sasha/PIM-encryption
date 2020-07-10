#include "pim_crypto.h"
#include "common.h"
#include <dpu.h>
#include <stdio.h>
#include <time.h>

#ifndef DPU_ENCRYPT_BINARY
#define DPU_ENCRYPT_BINARY "build/dpu_encrypt"
#endif

#ifndef DPU_DECRYPT_BINARY
#define DPU_DECRYPT_BINARY "build/dpu_decrypt"
#endif

int dpu_AES_ecb(void *in, void *out, unsigned long length, const void *key,
                int operation, unsigned int nr_of_dpus) {

  if (operation != OP_ENCRYPT && operation != OP_DECRYPT) {
    ERROR("Invalid operation\n");
    return -1;
  }

  if (length % 128 != 0) {
    ERROR("Length is not a multiple of block size\n");
    return -1;
  }

  clock_t times[9];

  struct dpu_set_t dpu_set;
  uint32_t real_nr_dpus;

  times[0] = clock(); // start

  if (nr_of_dpus != 0) {
    DPU_ASSERT(dpu_alloc(nr_of_dpus, NULL, &dpu_set));
  } else {
    int nr_of_dpus_wanted = length / MIN_CHUNK_SIZE;
    dpu_error_t error = dpu_alloc(nr_of_dpus_wanted, NULL, &dpu_set);

    if (error == DPU_ERR_ALLOCATION) {
      DPU_ASSERT(dpu_alloc(DPU_ALLOCATE_ALL, NULL, &dpu_set));
    } else {
      DPU_ASSERT(error);
    }
  }

  times[1] = clock(); // DPUs allocated

  DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &real_nr_dpus));
  int chunk_size = length / real_nr_dpus;

  if (chunk_size > MRAM_SIZE) { // More data than will fit in MRAM
    ERROR("Data does not fit in MRAM (%ld bytes into %d DPUs)\n", length, real_nr_dpus);
    DPU_ASSERT(dpu_free(dpu_set));
    return -1;
  }

  if (chunk_size % 128 != 0) { // Some blocks are not whole
    ERROR("Length is not a multiple of block size when split across %d DPUs\n", real_nr_dpus);
    DPU_ASSERT(dpu_free(dpu_set));
    return -1;
  }

  if (length % chunk_size != 0) { // Data does not fit evenly onto DPUs
    ERROR("%ld bytes cannot be split evenly across %d DPUs\n", length, real_nr_dpus);
    DPU_ASSERT(dpu_free(dpu_set));
    return -1;
  }

  DEBUG("Using %4.d DPU(s) %2.d tasklets, ", real_nr_dpus, NR_TASKLETS);

  if (operation == OP_ENCRYPT) {
    DPU_ASSERT(dpu_load(dpu_set, DPU_ENCRYPT_BINARY, NULL));
  } else {
    DPU_ASSERT(dpu_load(dpu_set, DPU_DECRYPT_BINARY, NULL));
  }

  times[2] = clock(); // DPUs loaded

  uint64_t offset = 0;

  struct dpu_set_t dpu;
  DPU_FOREACH(dpu_set, dpu) {
    DPU_ASSERT(dpu_prepare_xfer(dpu, in + offset));

    offset += chunk_size;
  }

  dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, XSTR(DPU_BUFFER), 0, chunk_size, DPU_XFER_DEFAULT);

  times[3] = clock(); // Data transferred to DPUs

  DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(KEY_BUFFER), 0, key, KEY_BUFFER_SIZE));
  DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(DPU_DATA_SIZE), 0, &chunk_size, sizeof(chunk_size)));

  times[4] = clock(); // Key and data size copied to DPUs

  DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

  times[5] = clock(); // DPUs launched, data encrypted

  DEBUG("%s took %3.2fs ",
         (operation == OP_ENCRYPT) ? "encryption" : "decryption",
         ((double) (times[5] - times[0])) / CLOCKS_PER_SEC);

  uint64_t cycles_min = 0;
  uint64_t cycles_max = 0;
  uint64_t cycles_avg = 0;
  offset = 0;

  DPU_FOREACH(dpu_set, dpu) {
    DPU_ASSERT(dpu_prepare_xfer(dpu, out + offset));

    offset += chunk_size;
  }

  dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, XSTR(DPU_BUFFER), 0, chunk_size, DPU_XFER_DEFAULT);

  times[6] = clock(); // Encrypted data copied from DPUs

  DPU_FOREACH(dpu_set, dpu) {
    uint64_t cycles;
    dpu_copy_from(dpu, "dpu_perfcount", 0, &cycles,
                  sizeof(cycles));

    cycles_min = (cycles_min == 0) ? cycles : cycles_min;
    cycles_min = (cycles_min < cycles) ? cycles_min : cycles;
    cycles_max = (cycles_max > cycles) ? cycles_max : cycles;
    cycles_avg += cycles;
  }

  cycles_avg /= real_nr_dpus;
  DEBUG("%10.ld cycles avg, %10.ld cycles min, %10.ld cycles max\n", cycles_avg, cycles_min, cycles_max);

  times[7] = clock(); // Performance counts retrieved

  DPU_ASSERT(dpu_free(dpu_set));

  times[8] = clock(); // DPUs freed

  // Parse and output the data for experiments
  // 
  // TODO: This is dirty, it shouldn't be done here. The plan is to use shell
  // scripts to run the binary multiple times, so the header needs to be
  // printed by the script instead. But having the header and data format
  // string in different places makes them prone to misalignment. This also
  // makes it hard to change timing sections. A better way might be a
  // function with static arrays of timing data which automatically records
  // each consecutive measurement to the next index, plus another function
  // for processing and outputting everything.

  MEASURE("Tasklets,DPUs,Operation,Data size,Allocation time,Loading time,Data copy in,Parameter copy in,Launch,Data copy out,Performance count copy out,Free DPUs,Performance count min, max, average\n");
  double times_adjusted[9];

  for (int i = 1; i < 9; i++) {
    times_adjusted[i] = (double)(times[i] - times[i-1]);
    times_adjusted[i] /= CLOCKS_PER_SEC;
  }

  MEASURE("%d,%d,%d,%ld,%f,%f,%f,%f,%f,%f,%f,%f,%ld,%ld,%ld\n", NR_TASKLETS, real_nr_dpus, operation, length, times_adjusted[1], times_adjusted[2], times_adjusted[3], times_adjusted[4], times_adjusted[5], times_adjusted[6], times_adjusted[7], times_adjusted[8], cycles_min, cycles_max, cycles_avg);

  return 0;
}
