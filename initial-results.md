# PIM bulk encryption initial results

The goal of this experiment was to estimate the rate at which one DPU could encrypt data with 128-bit AES. Additionally, I used minor modifications to measure the latency of DMA read instructions with various transfer sizes and the ratio of computing time to DMA transfer time in the encryption application. Note that this was an informal investigation, and results provided here were not rigorously tested.

Key results:
- DPUs can encrypt data at a rate of 550 cycles/byte, potentially 1500x slower than a host core with AES-NI
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

If one tasklet is always doing DMA and 16 others are doing crypto, the DMA should take 2400 cycles while the crypto takes (at least) about 100k cycles* - crypto will be the bottleneck. With the DPUs at 266MHz and an MRAM size of 64MiB, bulk encryption should take about 12 seconds (not counting loading data onto/off of the UPMEM DRAM).

*this assumes that when split among tasklets, the crypto will take the same ~100k instructions at about 1 instruction/cycle

At a speed of 267MHz, 640 DPUs will have a total throughput of roughly 300MiB/s  
At a speed of 2.4GHz, 16 CPU cores will have a total throughput of roughly 67MiB/s assuming the same rate of 550 cycles/byte

According to an [Intel paper on AES-NI](https://software.intel.com/sites/default/files/m/d/4/1/d/8/10TB24_Breakthrough_AES_Performance_with_Intel_AES_New_Instructions.final.secure.pdf), one thread on one core can manage 4.2 cycles/byte. Assuming the UPMEM machine runs at 3GHz and runs 16 threads on 16 cores, it may be able to manage 11.4GiB/s if memory were not a bottleneck.

So, the attractiveness of this use may really depend on how big a bottleneck memory bus contention is for the CPU.

