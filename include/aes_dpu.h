
#  define AES_MAXNR 14
struct aes_key_st {
#  ifdef AES_LONG
    unsigned long rd_key[4 * (AES_MAXNR + 1)];
#  else
    unsigned int rd_key[4 * (AES_MAXNR + 1)];
#  endif
    int rounds;
};
typedef struct aes_key_st AES_KEY;
