/*
 * Copyright 2020, 2021. Heekuck Oh, all rights reserved
 * 이 프로그램은 한양대학교 ERICA 소프트웨어학부 재학생을 위한 교육용으로 제작되었습니다.
 */
#include "miller_rabin.h"

/*
 * Miller-Rabin Primality Testing against small sets of bases
 *
 * if n < 2^64,
 * it is enough to test a = 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, and 37.
 *
 * if n < 3,317,044,064,679,887,385,961,981,
 * it is enough to test a = 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, and 41.
 */
const uint64_t a[ALEN] = {2,3,5,7,11,13,17,19,23,29,31,37};

/*
 * miller_rabin() - Miller-Rabin Primality Test (deterministic version)
 *
 * n > 3, an odd integer to be tested for primality
 * It returns 1 if n is prime, 0 otherwise.
 */
int miller_rabin(uint64_t n){
    uint64_t i, j, l, q=n-1, k=0, flg = PRIME;

    // a의 원소 => 소수
    for(i=0; i<ALEN; i++){
        if(n==a[i]) return PRIME;
    }

    // 1 or 짝수 => 소수x 
    if(n==1||n%2==0) return COMPOSITE;

    // 밀러라빈 알고리즘의 사용될 q와 k를 구함
    while(q%2==0){
        q/=2;
        k++;
    }
    
    // a의 모든 원소에대해 두 가지 조건 (a^q mod n = 1 or (a^2q)^j mod n = n-1) 확인
    for(i=0; i<ALEN; i++){
        flg = COMPOSITE;
        l = 1;
        if(mod_pow(a[i], q, n) == 1) flg = PRIME;
        for(j=0; j<k; j++){
            if(mod_pow(mod_pow(a[i], q, n), l, n) == n-1) flg = PRIME;
            l*=2;
        }
        if (flg==COMPOSITE) return COMPOSITE;
    }
    return PRIME;
}