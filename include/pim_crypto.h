#ifndef PIM_CRYPTO_H
#define PIM_CRYPTO_H

#define OP_ENCRYPT 1
#define OP_DECRYPT 2

#define MIN_CHUNK_SIZE (2 << 20)

int dpu_AES_ecb(const void *in, void *out, unsigned int length, const void *key,
                int operation);

#endif /* !PIM_CRYPTO_H */
