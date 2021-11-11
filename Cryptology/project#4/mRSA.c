/*
 * Copyright 2020, 2021. Heekuck Oh, all rights reserved
 * 이 프로그램은 한양대학교 ERICA 소프트웨어학부 재학생을 위한 교육용으로 제작되었습니다.
 */
#include <stdlib.h>
#include "mRSA.h"

const uint64_t a[ALEN] = {2,3,5,7,11,13,17,19,23,29,31,37};

// 유클리드 알고리즘을 사용하여 최대공약수 계산
static uint64_t gcd(uint64_t a, uint64_t b){
    while (b){
        uint64_t tmp = a;
        a = b;
        b = tmp%b;
    }
    return a;
}

// a^−1 mod m을 계산 (역이 존재하지 않는 경우 0을 리턴)
static uint64_t mul_inv(uint64_t a, uint64_t m){
    uint64_t d0=a, d1=m;
    uint64_t x0=1, x1=0;

    while (d1){
        uint64_t q = d0/d1;

        uint64_t tmp = d0;
        d0 = d1;
        d1 = tmp-q*d1;

        tmp  = x0;
        x0 = x1;
        x1 = tmp-q*x1;
    }
    if(d0==1) return (x0>>63 ? x0+m : x0);   
    else return 0;
}

// a+b mod m을 계산
static uint64_t mod_add(uint64_t a, uint64_t b, uint64_t m){
    a%=m; b%=m;
    if(m-b<=a) return a-(m-b);
    else return a+b;
}

// a*b mod m을 계산
static uint64_t mod_mul(uint64_t a, uint64_t b, uint64_t m){
    uint64_t r = 0;
    while (b > 0) {
        if (b & 1)
            r = mod_add(r, a, m);
        b = b >> 1;
        a = mod_add(a, a, m);
    }
    return r;
}

// a^b mod m을 계산
static uint64_t mod_pow(uint64_t a, uint64_t b, uint64_t m){
    uint64_t r = 1;
    while (b > 0) {
        if (b & 1)
            r = mod_mul(r, a, m);
        b = b >> 1;
        a = mod_mul(a, a, m);
    }
    return r;
}

// 64비트 정수 n이 소수이면 PRIME을, 합성수이면 COMPOSITE를 반환
static int miller_rabin(uint64_t n){
	if(n==2) return PRIME;
	if(n%2==0) return COMPOSITE;

	int i = 0;
	uint64_t k, pow;
	while (i < ALEN && a[i] < n-1){
		k=n-1;
		pow = mod_pow(a[i], k, n);
		if( pow !=1)
			return COMPOSITE;
		i++;
	}
	return PRIME;
}

/*
 * mRSA_generate_key() - generates mini RSA keys e, d and n
 * Carmichael's totient function Lambda(n) is used.
 */
void mRSA_generate_key(uint64_t *e, uint64_t *d, uint64_t *n){
    uint64_t p=0, q=0, Lambda;
    while (p*q < MINIMUM_N){
        while (1){
            arc4random_buf(&p, sizeof(uint32_t));
            if(miller_rabin(p)) break;
        }
        while (1){
            arc4random_buf(&q, sizeof(uint32_t));
            if(miller_rabin(q)) break;
        }
    }
    *n = p * q; // n 생성
    Lambda = ((p - 1)*(q - 1))/gcd(p - 1, q - 1);
    while (1){ // e, d 생성
        arc4random_buf(e,sizeof(uint64_t));
        if ((1 < *e) && (*e < Lambda) && (gcd(*e,Lambda) == 1)){
            *d = mul_inv(*e,Lambda);
            break;
        }   
    }
}

/*
 * mRSA_cipher() - compute m^k mod n
 * If data >= n then returns 1 (error), otherwise 0 (success).
 */
int mRSA_cipher(uint64_t *m, uint64_t k, uint64_t n){
    if(n<=*m) return 1;
    *m = mod_pow(*m, k, n);
    return 0;
}
