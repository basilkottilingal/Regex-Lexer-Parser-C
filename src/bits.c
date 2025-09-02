#include <stdint.h>

#include "bits.h"
  
#if !defined(__GNUC__) && !defined(__clang__)
int __builtin_clzll (unsigned long long k) {
  if ( k == 0ull ) return ULL_BITS;
  unsigned long long mask = 1ull << (ULL_BITS - 1); 
  int n = 0; while ( (k & mask) == 0ull) { n++; mask >>= 1; } return n;
}

int __builtin_ctzll (unsigned long long k) {
  if ( k == 0ull ) return ULL_BITS; 
  int n = 0;  while ((k & 1ull) == 0ull) { k >>= 1; n++; } return n;  
}

int __builtin_popcountll (unsigned long long k) {
  int n = 0, b;
  while (k) {
    ++n; k &= k-1;
  }
  return n;
}
#endif


