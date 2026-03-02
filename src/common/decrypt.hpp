#pragma once
// 2020.8.22

#include <stdint.h>
#include <stdlib.h>

namespace iris
{
	uint8_t* aes_init(size_t key_size);
	void aes_key_expansion(uint8_t* key, uint8_t* w);
	void aes_inv_cipher(uint8_t* in, uint8_t* out, uint8_t* w);
	void aes_cipher(uint8_t* in, uint8_t* out, uint8_t* w);

	/*
	uint8_t i;
	uint8_t key[] = {
		0x00, 0x01, 0x02, 0x03,
		0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b,
		0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13,
		0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b,
		0x1c, 0x1d, 0x1e, 0x1f};

	uint8_t in[] = {
		0x00, 0x11, 0x22, 0x33,
		0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xaa, 0xbb,
		0xcc, 0xdd, 0xee, 0xff};
	
	uint8_t out[16]; // 128
	uint8_t *w; // expanded key
	w = aes_init(sizeof(key));
	aes_key_expansion(key, w);

	printf("Plaintext message:\n");
	for (i = 0; i < 4; i++) {
		printf("%02x %02x %02x %02x ", in[4*i+0], in[4*i+1], in[4*i+2], in[4*i+3]);
	}*/
	//aes_cipher(in, out /* out */, w /* expanded key */);

	//printf("Ciphered message:\n");
	//for (i = 0; i < 4; i++) {
	//	printf("%02x %02x %02x %02x ", out[4 * i + 0], out[4 * i + 1], out[4 * i + 2], out[4 * i + 3]);
	//}
	//aes_inv_cipher(out, in, w);
	//printf("Original message (after inv cipher):\n");
	//for (i = 0; i < 4; i++) {
	//	printf("%02x %02x %02x %02x ", in[4 * i + 0], in[4 * i + 1], in[4 * i + 2], in[4 * i + 3]);
	//}
	//free(w);
//
//#include <stdint.h>     //for int8_t
//#include <string.h>     //for memcmp
//#include <wmmintrin.h>  //for intrinsics for AES-NI
////compile using gcc and following arguments: -g;-O0;-Wall;-msse2;-msse;-march=native;-maes
//
////internal stuff
//
////macros
//#define DO_ENC_BLOCK(m,k) \
//	do{\
//        m = _mm_xor_si128       (m, k[ 0]); \
//        m = _mm_aesenc_si128    (m, k[ 1]); \
//        m = _mm_aesenc_si128    (m, k[ 2]); \
//        m = _mm_aesenc_si128    (m, k[ 3]); \
//        m = _mm_aesenc_si128    (m, k[ 4]); \
//        m = _mm_aesenc_si128    (m, k[ 5]); \
//        m = _mm_aesenc_si128    (m, k[ 6]); \
//        m = _mm_aesenc_si128    (m, k[ 7]); \
//        m = _mm_aesenc_si128    (m, k[ 8]); \
//        m = _mm_aesenc_si128    (m, k[ 9]); \
//        m = _mm_aesenclast_si128(m, k[10]);\
//    }while(0)
//
//#define DO_DEC_BLOCK(m,k) \
//	do{\
//        m = _mm_xor_si128       (m, k[10+0]); \
//        m = _mm_aesdec_si128    (m, k[10+1]); \
//        m = _mm_aesdec_si128    (m, k[10+2]); \
//        m = _mm_aesdec_si128    (m, k[10+3]); \
//        m = _mm_aesdec_si128    (m, k[10+4]); \
//        m = _mm_aesdec_si128    (m, k[10+5]); \
//        m = _mm_aesdec_si128    (m, k[10+6]); \
//        m = _mm_aesdec_si128    (m, k[10+7]); \
//        m = _mm_aesdec_si128    (m, k[10+8]); \
//        m = _mm_aesdec_si128    (m, k[10+9]); \
//        m = _mm_aesdeclast_si128(m, k[0]);\
//    }while(0)
//
//#define AES_128_key_exp(k, rcon) aes_128_key_expansion(k, _mm_aeskeygenassist_si128(k, rcon))
//
//	static __m128i key_schedule[20];//the expanded key
//
//	static __m128i aes_128_key_expansion(__m128i key, __m128i keygened) {
//		keygened = _mm_shuffle_epi32(keygened, _MM_SHUFFLE(3, 3, 3, 3));
//		key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
//		key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
//		key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
//		return _mm_xor_si128(key, keygened);
//	}
//
//	//public API
//	void aes128_load_key(int8_t* enc_key) {
//		key_schedule[0] = _mm_loadu_si128((const __m128i*) enc_key);
//		key_schedule[1] = AES_128_key_exp(key_schedule[0], 0x01);
//		key_schedule[2] = AES_128_key_exp(key_schedule[1], 0x02);
//		key_schedule[3] = AES_128_key_exp(key_schedule[2], 0x04);
//		key_schedule[4] = AES_128_key_exp(key_schedule[3], 0x08);
//		key_schedule[5] = AES_128_key_exp(key_schedule[4], 0x10);
//		key_schedule[6] = AES_128_key_exp(key_schedule[5], 0x20);
//		key_schedule[7] = AES_128_key_exp(key_schedule[6], 0x40);
//		key_schedule[8] = AES_128_key_exp(key_schedule[7], 0x80);
//		key_schedule[9] = AES_128_key_exp(key_schedule[8], 0x1B);
//		key_schedule[10] = AES_128_key_exp(key_schedule[9], 0x36);
//
//		// generate decryption keys in reverse order.
//		// k[10] is shared by last encryption and first decryption rounds
//		// k[0] is shared by first encryption round and last decryption round (and is the original user key)
//		// For some implementation reasons, decryption key schedule is NOT the encryption key schedule in reverse order
//		key_schedule[11] = _mm_aesimc_si128(key_schedule[9]);
//		key_schedule[12] = _mm_aesimc_si128(key_schedule[8]);
//		key_schedule[13] = _mm_aesimc_si128(key_schedule[7]);
//		key_schedule[14] = _mm_aesimc_si128(key_schedule[6]);
//		key_schedule[15] = _mm_aesimc_si128(key_schedule[5]);
//		key_schedule[16] = _mm_aesimc_si128(key_schedule[4]);
//		key_schedule[17] = _mm_aesimc_si128(key_schedule[3]);
//		key_schedule[18] = _mm_aesimc_si128(key_schedule[2]);
//		key_schedule[19] = _mm_aesimc_si128(key_schedule[1]);
//	}
//
//	void aes128_enc(int8_t* plainText, int8_t* cipherText) {
//		__m128i m = _mm_loadu_si128((__m128i*) plainText);
//
//		DO_ENC_BLOCK(m, key_schedule);
//
//		_mm_storeu_si128((__m128i*) cipherText, m);
//	}
//
//	void aes128_dec(int8_t* cipherText, int8_t* plainText) {
//		__m128i m = _mm_loadu_si128((__m128i*) cipherText);
//
//		DO_DEC_BLOCK(m, key_schedule);
//
//		_mm_storeu_si128((__m128i*) plainText, m);
//	}
//
//	//return 0 if no error
//	//1 if encryption failed
//	//2 if decryption failed
//	//3 if both failed
//	int aes128_self_test(void) {
//		int8_t plain[] = { 0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d, 0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34 };
//		int8_t enc_key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
//		int8_t cipher[] = { 0x39, 0x25, 0x84, 0x1d, 0x02, 0xdc, 0x09, 0xfb, 0xdc, 0x11, 0x85, 0x97, 0x19, 0x6a, 0x0b, 0x32 };
//		int8_t computed_cipher[16];
//		int8_t computed_plain[16];
//		int out = 0;
//		aes128_load_key(enc_key);
//		aes128_enc(plain, computed_cipher);
//		aes128_dec(cipher, computed_plain);
//		if (memcmp(cipher, computed_cipher, sizeof(cipher))) out = 1;
//		if (memcmp(plain, computed_plain, sizeof(plain))) out |= 2;
//		return out;
//	}

}