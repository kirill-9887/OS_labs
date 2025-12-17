#include "../include/library.h"

// NOTE: MSVC compiler does not export symbols unless annotated
#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

EXPORT int prime_count(int a, int b) {
    int res = 0;
    for (int x = a; x <= b; ++x) {
        if (x < 2) {
            continue;
        }
        int is_prime = 1;
        for (int d = 2; d * d <= x; ++d) {
            if (x % d == 0) {
                is_prime = 0;
                break;
            }
        }
        res += is_prime;
    }
    return res;
}

EXPORT int gcd(int a, int b) {
    while (b) {
        int tmp = a;
        a = b;
        b = tmp % b;
    }
    return a;
}