#include "crypto.h"
#include "common.h"
#include "PIM-common/common/include/common.h"
#include "PIM-common/host/include/host.h"
#include <dpu.h>
#include <stdio.h>
#include <time.h>

int dpu_AES_ecb(void *in, void *out, unsigned long length, const void *key_ptr,
                int operation, unsigned int nr_of_dpus) {

  if (operation != OP_ENCRYPT && operation != OP_DECRYPT) {
    ERROR("Invalid operation\n");
    return -1;
  }

  if (length % AES_BLOCK_SIZE_BYTES != 0) {
    ERROR("Length is not a multiple of block size\n");
    return -1;
  }

  struct timespec times[9];

  struct dpu_set_t dpu_set;

  clock_gettime(CLOCK_MONOTONIC_RAW, times); // start

  DPU_ASSERT(dpu_alloc(nr_of_dpus, NULL, &dpu_set));

  clock_gettime(CLOCK_MONOTONIC_RAW, times+1); // DPUs allocated

  DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
  int chunk_size = length / nr_of_dpus;

  if (chunk_size > MRAM_SIZE) { // More data than will fit in MRAM
    ERROR("Data does not fit in MRAM (%ld bytes into %d DPUs)\n", length, nr_of_dpus);
    DPU_ASSERT(dpu_free(dpu_set));
    return -1;
  }

  if (chunk_size % AES_BLOCK_SIZE_BYTES != 0) { // Some blocks are not whole
    ERROR("Length is not a multiple of block size when split across %d DPUs\n", nr_of_dpus);
    DPU_ASSERT(dpu_free(dpu_set));
    return -1;
  }

  if (length % chunk_size != 0) { // Data does not fit evenly onto DPUs
    ERROR("%ld bytes cannot be split evenly across %d DPUs\n", length, nr_of_dpus);
    DPU_ASSERT(dpu_free(dpu_set));
    return -1;
  }

  DEBUG("Using %4.d DPU(s) %2.d tasklets, ", nr_of_dpus, NR_TASKLETS);

  if (operation == OP_ENCRYPT) {
    DPU_ASSERT(dpu_load(dpu_set, DPU_ENCRYPT_BINARY, NULL));
  } else {
    DPU_ASSERT(dpu_load(dpu_set, DPU_DECRYPT_BINARY, NULL));
  }

  clock_gettime(CLOCK_MONOTONIC_RAW, times+2); // DPUs loaded

  uint64_t offset = 0;

  struct dpu_set_t dpu;
  DPU_FOREACH(dpu_set, dpu) {

#ifndef NOBULK
    DPU_ASSERT(dpu_prepare_xfer(dpu, in + offset));
#else
    DPU_ASSERT(
        dpu_copy_to(dpu, XSTR(DPU_DATA_BUFFER), 0, in + offset, chunk_size));
#endif

    offset += chunk_size;
  }

#ifndef NOBULK
  dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, XSTR(DPU_DATA_BUFFER), 0, chunk_size, DPU_XFER_DEFAULT);
#endif

  clock_gettime(CLOCK_MONOTONIC_RAW, times+3); // Data transferred to DPUs

  DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(DPU_KEY_BUFFER), 0, key_ptr, DPU_KEY_BUFFER_SIZE));
  DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(DPU_LENGTH_BUFFER), 0, &chunk_size, sizeof(chunk_size)));

  clock_gettime(CLOCK_MONOTONIC_RAW, times+4); // Key and data size copied to DPUs

  DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

  clock_gettime(CLOCK_MONOTONIC_RAW, times+5); // DPUs launched, data encrypted

  DEBUG("%s took %3.2fs ",
         (operation == OP_ENCRYPT) ? "encryption" : "decryption",
         TIME_DIFFERENCE(times[0], times[5]) );

  uint64_t perfcount_min = 0;
  uint64_t perfcount_max = 0;
  uint64_t perfcount_avg = 0;
  offset = 0;

  DPU_FOREACH(dpu_set, dpu) {

#ifndef NOBULK
    DPU_ASSERT(dpu_prepare_xfer(dpu, out + offset));
#else
    DPU_ASSERT(
        dpu_copy_from(dpu, XSTR(DPU_DATA_BUFFER), 0, out + offset, chunk_size));
#endif

    offset += chunk_size;
  }

#ifndef NOBULK
  dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, XSTR(DPU_DATA_BUFFER), 0, chunk_size, DPU_XFER_DEFAULT);
#endif

  clock_gettime(CLOCK_MONOTONIC_RAW, times+6); // Encrypted data copied from DPUs

  DPU_FOREACH(dpu_set, dpu) {
    uint64_t perfcount;
    dpu_copy_from(dpu, "dpu_perfcount", 0, &perfcount, sizeof(perfcount));

    perfcount_min = (perfcount_min == 0) ? perfcount : perfcount_min;
    perfcount_min = (perfcount_min < perfcount) ? perfcount_min : perfcount;
    perfcount_max = (perfcount_max > perfcount) ? perfcount_max : perfcount;
    perfcount_avg += perfcount;
  }

  perfcount_avg /= nr_of_dpus;
  DEBUG("Performance count %s %10.ld avg, %10.ld min, %10.ld max\n",
        XSTR(PERFCOUNT_TYPE), perfcount_avg, perfcount_min, perfcount_max);

  clock_gettime(CLOCK_MONOTONIC_RAW, times+7); // Performance counts retrieved

  DPU_ASSERT(dpu_free(dpu_set));

  clock_gettime(CLOCK_MONOTONIC_RAW, times+8); // DPUs freed

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

  // MEASURE("Tasklets,DPUs,Operation,Data size,Allocation time,Loading
  // time,Data copy in,Parameter copy in,Launch,Data copy out, Performance count
  // copy out,Free DPUs,Performance count type,Performance count min, max,
  // average\n");

#ifdef EXPERIMENT
  double times_adjusted[9];
  for (int i = 1; i < 9; i++) {
    times_adjusted[i] = TIME_DIFFERENCE(times[i-1], times[i]);
  }

  MEASURE("%d,%d,%d,%ld,%.12f,%.12f,%.12f,%.12f,%.12f,%.12f,%.12f,%.12f,%s,%ld,%ld,%ld\n", NR_TASKLETS,
          nr_of_dpus, operation, length, times_adjusted[1], times_adjusted[2],
          times_adjusted[3], times_adjusted[4], times_adjusted[5],
          times_adjusted[6], times_adjusted[7], times_adjusted[8],
          XSTR(PERFCOUNT_TYPE), perfcount_min, perfcount_max, perfcount_avg);
#endif

  return 0;
}
