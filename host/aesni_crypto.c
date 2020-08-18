#include "PIM-common/host/include/host.h"
#include "common.h"
#include "crypto.h"
#include <limits.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <string.h>

void handleErrors(void) {
  ERR_print_errors_fp(stderr);
  abort();
}

int aesni_AES_ecb(void *in, void *out, unsigned long length,
                  const void *key_ptr, int operation) {
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);

  // OpenSSL only accepts an integer length, so we reject lengths any longer
  // than that
  if (length > INT_MAX) {
    return EINVAL;
  }

  if (operation == OP_ENCRYPT) {
    operation = 1;
  } else if (operation == OP_DECRYPT) {
    operation = 0;
  } else {
    return EINVAL;
  }

  EVP_CIPHER_CTX *ctx;

  if (!(ctx = EVP_CIPHER_CTX_new())) {
    handleErrors();
  }

  if (1 != EVP_CipherInit_ex(ctx, EVP_aes_128_ecb(), NULL, key_ptr, NULL,
                             operation)) {
    handleErrors();
  }

  int outl;

  if (1 != EVP_CipherUpdate(ctx, out, &outl, in, length)) {
    handleErrors();
  }

  if (length != (unsigned int)outl) {
    ERROR("Error: OpenSSL did not encrypt all data\n");
    EVP_CIPHER_CTX_free(ctx);
    return -1;
  }

  EVP_CIPHER_CTX_free(ctx);

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  double execution_time = TIME_DIFFERENCE(start, end);

  // TODO: add a cycle count
  // Operation, Data size, Execution time
  MEASURE("%d,%ld,%.12f\n", operation, length, execution_time);

  DEBUG("%sed %ld bytes in %fs\n", (operation == 1) ? "Encrypt" : "Decrypt",
        length, execution_time);

  return 0;
}
