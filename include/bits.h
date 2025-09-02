#ifndef _BITS_H_
#define _BITS_H_
    
  #include <limits.h>

  #define ULL_BITS    (sizeof(unsigned long long) * CHAR_BIT)
  #define SIZE_T_BITS (sizeof(size_t) * CHAR_BIT)
  
  #if !defined(__GNUC__) && !defined(__clang__)

    int __builtin_clzll      (unsigned long long k);
    int __builtin_ctzll      (unsigned long long k);
    int __builtin_popcountll (unsigned long long k);
  
  #endif

#endif
