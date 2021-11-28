/*
 * Copyright 2020,2021. Heekuck Oh, all rights reserved
 * 이 프로그램은 한양대학교 ERICA 소프트웨어학부 재학생을 위한 교육용으로 제작되었습니다.
 */
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#include "rsa_pss.h"

#if defined(SHA224)
void (*sha)(const unsigned char *, unsigned int, unsigned char *) = sha224;
#elif defined(SHA256)
void (*sha)(const unsigned char *, unsigned int, unsigned char *) = sha256;
#elif defined(SHA384)
void (*sha)(const unsigned char *, unsigned int, unsigned char *) = sha384;
#else
void (*sha)(const unsigned char *, unsigned int, unsigned char *) = sha512;
#endif

/*
 * Copyright 2020, 2021. Heekuck Oh, all rights reserved
 * rsa_generate_key() - generates RSA keys e, d and n in octet strings.
 * If mode = 0, then e = 65537 is used. Otherwise e will be randomly selected.
 * Carmichael's totient function Lambda(n) is used.
 */
void rsa_generate_key(void *_e, void *_d, void *_n, int mode)
{
    mpz_t p, q, lambda, e, d, n, gcd;
    gmp_randstate_t state;
    
    /*
     * Initialize mpz variables
     */
    mpz_inits(p, q, lambda, e, d, n, gcd, NULL);
    gmp_randinit_default(state);
    gmp_randseed_ui(state, arc4random());
    /*
     * Generate prime p and q such that 2^(RSAKEYSIZE-1) <= p*q < 2^RSAKEYSIZE
     */
    do {
        do {
            mpz_urandomb(p, state, RSAKEYSIZE/2);
            mpz_setbit(p, 0);
            mpz_setbit(p, RSAKEYSIZE/2-1);
       } while (mpz_probab_prime_p(p, 50) == 0);
        do {
            mpz_setbit(q, 0);
            mpz_setbit(q, RSAKEYSIZE/2-1);
            mpz_urandomb(q, state, RSAKEYSIZE/2);
        } while (mpz_probab_prime_p(q, 50) == 0);
        mpz_mul(n, p, q);
    } while (!mpz_tstbit(n, RSAKEYSIZE-1));
    /*
     * Generate e and d using Lambda(n)
     */
    mpz_sub_ui(p, p, 1);
    mpz_sub_ui(q, q, 1);
    mpz_lcm(lambda, p, q);
    if (mode == 0)
        mpz_set_ui(e, 65537);
    else do {
        mpz_urandomb(e, state, RSAKEYSIZE);
        mpz_gcd(gcd, e, lambda);
    } while (mpz_cmp(e, lambda) >= 0 || mpz_cmp_ui(gcd, 1) != 0);
    mpz_invert(d, e, lambda);
    /*
     * Convert mpz_t values into octet strings
     */
    mpz_export(_e, NULL, 1, RSAKEYSIZE/8, 1, 0, e);
    mpz_export(_d, NULL, 1, RSAKEYSIZE/8, 1, 0, d);
    mpz_export(_n, NULL, 1, RSAKEYSIZE/8, 1, 0, n);
    /*
     * Free the space occupied by mpz variables
     */
    mpz_clears(p, q, lambda, e, d, n, gcd, NULL);
}

/*
 * Copyright 2020. Heekuck Oh, all rights reserved
 * rsa_cipher() - compute m^k mod n
 * If m >= n then returns EM_MSG_OUT_OF_RANGE, otherwise returns 0 for success.
 */
static int rsa_cipher(void *_m, const void *_k, const void *_n)
{
    mpz_t m, k, n;
    
    /*
     * Initialize mpz variables
     */
    mpz_inits(m, k, n, NULL);
    /*
     * Convert big-endian octets into mpz_t values
     */
    mpz_import(m, RSAKEYSIZE/8, 1, 1, 1, 0, _m);
    mpz_import(k, RSAKEYSIZE/8, 1, 1, 1, 0, _k);
    mpz_import(n, RSAKEYSIZE/8, 1, 1, 1, 0, _n);
    /*
     * Compute m^k mod n
     */
    if (mpz_cmp(m, n) >= 0) {
        mpz_clears(m, k, n, NULL);
        return EM_MSG_OUT_OF_RANGE;
    }
    mpz_powm(m, m, k, n);
    /*
     * Convert mpz_t m into the octet string _m
     */
    mpz_export(_m, NULL, 1, RSAKEYSIZE/8, 1, 0, m);
    /*
     * Free the space occupied by mpz variables
     */
    mpz_clears(m, k, n, NULL);
    return 0;
}

/*
 * Copyright 2020. Heekuck Oh, all rights reserved
 * A mask generation function based on a hash function
 */
static unsigned char *mgf(const unsigned char *mgfSeed, size_t seedLen, unsigned char *mask, size_t maskLen)
{
    uint32_t i, count, c;
    size_t hLen;
    unsigned char *mgfIn, *m;
    
    /*
     * Check if maskLen > 2^32*hLen
     */
    hLen = SHASIZE/8;
    if (maskLen > 0x0100000000*hLen)
        return NULL;
    /*
     * Generate octet string mask
     */
    if ((mgfIn = (unsigned char *)malloc(seedLen+4)) == NULL)
        return NULL;;
    memcpy(mgfIn, mgfSeed, seedLen);
    count = maskLen/hLen + (maskLen%hLen ? 1 : 0);
    if ((m = (unsigned char *)malloc(count*hLen)) == NULL)
        return NULL;
    /*
     * Convert i to an octet string C of length 4 octets
     * Concatenate the hash of the seed mgfSeed and C to the octet string T:
     *       T = T || Hash(mgfSeed || C)
     */
    for (i = 0; i < count; i++) {
        c = i;
        mgfIn[seedLen+3] = c & 0x000000ff; c >>= 8;
        mgfIn[seedLen+2] = c & 0x000000ff; c >>= 8;
        mgfIn[seedLen+1] = c & 0x000000ff; c >>= 8;
        mgfIn[seedLen] = c & 0x000000ff;
        (*sha)(mgfIn, seedLen+4, m+i*hLen);
    }
    /*
     * Copy the mask and free memory
     */
    memcpy(mask, m, maskLen);
    free(mgfIn); free(m);
    return mask;
}

/*
 * rsassa_pss_sign - RSA Signature Scheme with Appendix 
 * RSA 서명
 */
int rsassa_pss_sign(const void *m, size_t mLen, const void *d, const void *n, void *s)
{
    unsigned char mHash[SHASIZE/8];
    unsigned char salt[SHASIZE/8];
    unsigned char Mprime[8+2*(SHASIZE/8)];
    unsigned char DB[RSAKEYSIZE/8-SHASIZE/8-1];
    unsigned char MGF[RSAKEYSIZE/8-SHASIZE/8-1];
    unsigned char maskedDB[RSAKEYSIZE/8-SHASIZE/8-1];
    unsigned char H[SHASIZE/8];
    unsigned char EM[RSAKEYSIZE/8];
    uint8_t msb = 0x01;
    uint8_t bc = 0xbc; 

    // 해시함수의 입력 데이터가 너무 길어 한도를 초과한 경우 
    // SHASIZE가 224 or 256의 경우 mLen이 64 bits를 넘으면 안됨
    if((SHASIZE == 224 || SHASIZE == 256) && (0x1fffffffffffffff<mLen))
        return EM_MSG_TOO_LONG;
    
    // 해시의 길이가 너무 커서 EM에서 수용할 수 없는 경우 
    if(RSAKEYSIZE/8 < 2*(SHASIZE/8)+2)
        return EM_HASH_TOO_LONG;
    
    // M' (Mprime)
    for(int i=0; i<8; i++) Mprime[i]=0x00;
    
    // mHash
    sha(m, mLen, mHash);
    memcpy(Mprime+8, mHash, SHASIZE/8);

    // salt
    arc4random_buf(salt, SHASIZE/8);
    memcpy(Mprime+8+SHASIZE/8, salt, SHASIZE/8);

    // H
    sha(Mprime, 8+2*(SHASIZE/8), H);

    // DB
    for(int i=0; i<RSAKEYSIZE/8-2*(SHASIZE/8)-1; i++) DB[i]=0x00;
    memcpy(DB+RSAKEYSIZE/8-2*(SHASIZE/8)-2, &msb, 1);
    memcpy(DB+RSAKEYSIZE/8-2*(SHASIZE/8)-1, salt, SHASIZE/8);

    //MGF
    mgf(H, SHASIZE/8, MGF, RSAKEYSIZE/8-SHASIZE/8-1);

    // maskedDB
    for(int i=0; i<RSAKEYSIZE/8-SHASIZE/8-1; i++){
        maskedDB[i] = DB[i]^MGF[i];
        EM[i] = maskedDB[i];
    }

    // EM
    if((EM[0]>>7) & 1) EM[0] = 0x00;
    memcpy(EM+RSAKEYSIZE/8-SHASIZE/8-1, H, SHASIZE/8);
    memcpy(EM+RSAKEYSIZE/8-1, &bc, 1);

    // s
    // 데이터 값이 모듈러스 n 보다 크거나 같은 경우 
    if(rsa_cipher(EM, d, n)) 
        return EM_MSG_OUT_OF_RANGE;
    memcpy(s, EM, RSAKEYSIZE/8);

    return 0;
}

/*
 * rsassa_pss_verify - RSA Signature Scheme with Appendix (
 * RSA 검증
 */
int rsassa_pss_verify(const void *m, size_t mLen, const void *e, const void *n, const void *s)
{
    unsigned char salt[SHASIZE/8];
    unsigned char mHash[SHASIZE/8];
    unsigned char Mprime[8+2*(SHASIZE/8)];
    unsigned char MHash[SHASIZE/8];
    unsigned char H[SHASIZE/8];
    unsigned char DB[RSAKEYSIZE/8-SHASIZE/8-1];
    unsigned char maskedDB[RSAKEYSIZE/8-SHASIZE/8-1];
    unsigned char EM[RSAKEYSIZE/8];
    unsigned char HMGF[SHASIZE/8];
    
    // EM (s^e mod n = EM)
    memcpy(EM, s, RSAKEYSIZE/8);
    if(rsa_cipher(EM, e, n)) 
        return EM_MSG_OUT_OF_RANGE;
    
    // EM의 처음 (MSB) 비트가 0이 아닌 경우 
    if(EM[0]>>7)   
        return EM_INVALID_INIT;
    
    // EM의 마지막 (LSB) 바이트가 0xBC가 아닌 경우
    if(EM[RSAKEYSIZE/8-1]^0xbc)
        return EM_INVALID_LAST;

    // maskedDB
    memcpy(maskedDB, EM, RSAKEYSIZE/8-SHASIZE/8-1);

    // H
    memcpy(H, EM+RSAKEYSIZE/8-SHASIZE/8-1, SHASIZE/8);

    // MGF(H)
    mgf(H, SHASIZE/8, HMGF, RSAKEYSIZE/8-SHASIZE/8-1);

    // DB
    DB[0] = 0x00;
    for(int i=1; i<RSAKEYSIZE/8-SHASIZE/8-1; i++)
        DB[i] = maskedDB[i]^HMGF[i];

    // DB의 앞 부분이 0x0000..00||0x01과 일치하지 않는 경우
    for(int i=0; i<RSAKEYSIZE/8-2*(RSAKEYSIZE/8)-2; i++){
        if(DB[i] ^ 0x00)
            return EM_INVALID_PD2;
    }
    if(DB[RSAKEYSIZE/8-2*SHASIZE/8-2] ^ 0x01)
        return EM_INVALID_PD2;

    //salt
    memcpy(salt, DB+RSAKEYSIZE/8-2*(SHASIZE/8)-1, SHASIZE/8);

    // mHash
    sha(m, mLen, mHash);

    // M'(Mprime)
    for(int i = 0; i < 8; i++) Mprime[i] = 0x00;
    memcpy(Mprime+8, mHash, SHASIZE/8);
    memcpy(Mprime+8+SHASIZE/8, salt, SHASIZE/8);

    // H(M')
    sha(Mprime, 8+2*(SHASIZE/8), MHash);

    for(int i=0; i<SHASIZE/8; i++){
        // 해시 값이 일치하지 않는 경우
        if(H[i]^MHash[i]) 
            return EM_HASH_MISMATCH;
    }
    return 0;
}
