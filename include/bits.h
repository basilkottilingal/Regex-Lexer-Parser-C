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

  #define BITBYTES(_b)            ( ((_b + 63) & ~(int)63) >> 3 )
  #define BITCLEAR(_B,_s)         memset (_B, 0, _s);
  #define BITLOOKUP(_B,_b)                                           \
    ( _B[(_b)>>6] &  ((uint64_t)  1 << ((_b) & (int) 63)))
  #define BITINSERT(_B,_b)                                           \
      _B[(_b)>>6] |= ((uint64_t)  1 << ((_b) & (int) 63))
  #define BITMINUS(_u,_v,_w,_s)                                      \
    for (int _i = 0; _i < (_s>>3); ++_i) {                           \
      _w[_i] = _u[_i] & ~_v[_i];                                     \
    }
  #define BITAND(_u,_v,_w,_c,_s)  do { _c = 0;                       \
    for (int _i = 0; _i < (_s>>3); ++_i) {                           \
      _c |= (_w[_i] = _u[_i] & _v[_i]) != 0;                         \
    } } while (0)
  #define BITCMP(_u,_v,_c,_s)     do { _c = 1;                       \
    for (int _i = 0; _c && _i < (_s>>3); ++_i) {                     \
      _c = (_u[_i] == _v[_i]);                                       \
    } } while (0)
  #define BITCOUNT(_B,_c,_s)      do { _c = 0;                       \
    for (int _i = 0; _i < (_s>>3); ++_i) {                           \
      _c += __builtin_popcountll( _B[_i] );                          \
    } } while (0)

#endif
