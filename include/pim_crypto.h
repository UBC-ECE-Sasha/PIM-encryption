#ifndef PIM_CRYPTO_H
#define PIM_CRYPTO_H

#define OP_ENCRYPT 1
#define OP_DECRYPT 2

#define MIN_CHUNK_SIZE (2 << 20)

int dpu_AES_ecb(void *in, void *out, unsigned long length, const void *key,
                int operation, unsigned int nr_of_dpus);

#endif /* !PIM_CRYPTO_H */
