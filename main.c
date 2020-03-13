#include <stdio.h>
#include "aes_core.c"
#include "aes_local.h"

int main(void) {
	char key[] = "hello world hello world";
	AES_KEY encrypt_key;
	AES_KEY decrypt_key;

	char plaintext[] = "Will the DPU be able to do ecryption/decryption? Will it make sense for it to do so? We will find out!";
	char ciphertext[256];
	char plaintext_decrypted[256];

	AES_set_encrypt_key(key, 128, &encrypt_key);
	AES_set_decrypt_key(key, 128, &decrypt_key);

	AES_encrypt(plaintext, ciphertext, &encrypt_key);
	AES_decrypt(ciphertext, plaintext_decrypted, &decrypt_key);

	printf("Encrypted result: %s\nDecrypted result: %s\n", ciphertext, plaintext_decrypted);
	return 0;
}

