#ifndef TINY_AES_H
#define TINY_AES_H
/* tiny-aes-c: public domain AES-128/192/256
   https://github.com/kokke/tiny-AES-c
   Stripped to AES-128 CBC only for scorpion OS               */

#include <stdint.h>
#include <stddef.h>

#define AES_BLOCKLEN 16
#define AES_KEYLEN   16
#define AES_keyExpSize 176

struct AES_ctx {
    uint8_t RoundKey[AES_keyExpSize];
    uint8_t Iv[AES_BLOCKLEN];
};

void AES_init_ctx_iv(struct AES_ctx *ctx, const uint8_t *key, const uint8_t *iv);
void AES_CBC_encrypt_buffer(struct AES_ctx *ctx, uint8_t *buf, size_t length);
void AES_CBC_decrypt_buffer(struct AES_ctx *ctx, uint8_t *buf, size_t length);

#endif /* TINY_AES_H */
