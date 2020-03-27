#ifndef AES_CORE_H
#define AES_CORE_H

#define AES_MAXNR 14
struct aes_key_st {
#ifdef AES_LONG
  unsigned long rd_key[4 * (AES_MAXNR + 1)];
#else
  unsigned int rd_key[4 * (AES_MAXNR + 1)];
#endif
  int rounds;
};
typedef struct aes_key_st AES_KEY;

/**
 * Expand the cipher key into the encryption key schedule.
 */
int AES_set_encrypt_key(const unsigned char *userKey, const int bits,
                        AES_KEY *key);

/**
 * Expand the cipher key into the decryption key schedule.
 */
int AES_set_decrypt_key(const unsigned char *userKey, const int bits,
                        AES_KEY *key);

/*
 * Encrypt a single block
 * in and out can overlap
 */
void AES_encrypt(const unsigned char *in, unsigned char *out,
                 const AES_KEY *key);

/*
 * Decrypt a single block
 * in and out can overlap
 */
void AES_decrypt(const unsigned char *in, unsigned char *out,
                 const AES_KEY *key);

#endif /* !AES_CORE_H */
