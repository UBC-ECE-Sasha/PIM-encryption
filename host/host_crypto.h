#ifndef HOST_CRYPTO_H
#define HOST_CRYPTO_H

#include "pim_crypto.h"

int host_AES_ecb(void *in, void *out, unsigned long length, const void *key_ptr,
                 int operation);

int host_AES_ecb_encrypt(void *in, void *out, unsigned long length,
                         const void *key_ptr);
int host_AES_ecb_decrypt(void *in, void *out, unsigned long length,
                         const void *key_ptr);
#endif /* !HOST_CRYPTO_H */
