# PIM bulk encryption initial results

The goal of this experiment was to estimate the rate at which one DPU could encrypt data with 128-bit AES. Additionally, I used minor modifications to measure the latency of DMA read instructions with various transfer sizes and the ratio of computing time to DMA transfer time in the encryption application. Note that this was an informal investigation, and results provided here were not rigorously tested.

Key results:
- DPUs may be able to encrypt data at a rate of up to 51 cycles/byte, potentially 72x slower than a host core with AES-NI
- DMA reads of 2KB take 1200 cycles to finish. Transfers have an overhead of about 130 cycles, and then transfer data at about 2 bytes/cycle.
- In an encryption application, DMA transfer time is negligible compared to computing time and will not be a bottleneck except when handling very small amounts of data.

## Methods
The experiment was performed on `upmemcloud3`, a machine with 640 DPUs in 10 physical ranks running at 266MHz. The code was compiled and run with UPMEM SDK version 2020.1.0.

Measurements were taken on revision `8bd449` by placing calls to `perfcounter_get` before and after the code of interest, and running two versions: one configured to measure instructions, and the other to measure cycles. No wall time measurements were made - these results rely only on the DPU's performance counter.

## Results
### Encryption
Measurements taken between lines 22 and 32 of dpu/dpu.c, inclusive.

Operation | Cycles | Instructions
---|---|---
Encryption | 1.13m | 103k
Decryption | 1.12m | 103k

#### Excluding DMA time
Measurements taken between lines 23 and 31 of dpu/dpu.c.

Operation | Cycles | Instructions
---|---|---
Encryption | 1.12m | 102k
Decryption | 1.10m | 99.3k

### DMA latency
Measurements taken of a single call to `mram_read` (line 22 of dpu/dpu.c).

Transfer size (B) | latency (cycles) | Bytes transferred/cycle
-------|------------|------------
2048 | 1200 | 1.7
1024 | 656 | 1.56
512 | 400 | 1.28
256 | 272 | 0.94

## Conclusions

Assuming that a properly parallelized algorithm can reach one instruction per cycle, 103k instructions divided by 2048B of data encrypted indicates that a DPU can encrypt data at a rate of 50.3 cycles/byte using 128-bit AES. Since DMA time is less than one cycle per byte, DMA will not be a bottleneck.

At a speed of 500MHz, 640 DPUs will have a total throughput of roughly 5.96GiB/s (assuming 50.3 cycles/byte per DPU)
At a speed of 3GHz, 16 host cores will have a total throughput of roughly 10.6GiB/s (assuming 4.2 cycles/byte per core; see [the Intel paper on AES-NI](https://software.intel.com/sites/default/files/m/d/4/1/d/8/10TB24_Breakthrough_AES_Performance_with_Intel_AES_New_Instructions.final.secure.pdf)

