#include "../include/library.h"

// NOTE: MSVC compiler does not export symbols unless annotated
#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

EXPORT int prime_count(int a, int b) {
    int res = 0;
    static int sieve[MAX_NUMBER] = {0};
    static int is_init = 0;
    if (!is_init) {
        sieve[0] = sieve[1] = 1;
        for (int i = 2; i < MAX_NUMBER; ++i) {
            if (sieve[i] == 0 && i <= MAX_NUMBER / i) {
                for (int j = i * i; j < MAX_NUMBER; j += i) {
                    sieve[j] = 1;
                }
            }
        }
    }
    for (int x = a; x <= b; ++x) {
        res += (int)(sieve[x] == 0);
    }
    return res;
}

EXPORT int gcd(int a, int b) {
    if (b < a) {
        int tmp = a;
        a = b;
        b = tmp;
    }
    for (int i = a; i > 1; --i) {
        if (a % i == 0 && b % i == 0) {
            return i;
        }
    }
    return 1;
}