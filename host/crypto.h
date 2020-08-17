#ifndef CRYPTO_H
#define CRYPTO_H

#define OP_ENCRYPT 1
#define OP_DECRYPT 2

#define MIN_CHUNK_SIZE (2 << 20)

int dpu_AES_ecb(void *in, void *out, unsigned long length, const void *key_ptr,
                int operation, unsigned int nr_of_dpus);
int host_AES_ecb(void *in, void *out, unsigned long length, const void *key_ptr,
                 int operation);
int aesni_AES_ecb(void *in, void *out, unsigned long length,
                  const void *key_ptr, int operation);

int host_AES_ecb_encrypt(void *in, void *out, unsigned long length,
                         const void *key_ptr);
int host_AES_ecb_decrypt(void *in, void *out, unsigned long length,
                         const void *key_ptr);

int aesni_AES_ecb_encrypt(void *in, void *out, unsigned long length,
                          const void *key_ptr);
int aesni_AES_ecb_decrypt(void *in, void *out, unsigned long length,
                          const void *key_ptr);

#endif /* !HOST_CRYPTO_H */
