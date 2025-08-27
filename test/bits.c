/*
.. test case for stack of bits.
.. $ make test-bits.s && ./test-bits
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "stack.h"

int main () {
  Stack * s = stack_new ( 512>>3 );
  int ibits [] = {12, 12, 0, 511, 100, 511, 88};
  printf ("\nInserting bits @ index [");
  for (int i=0; i<sizeof (ibits)/sizeof(int); ++i) {
    printf (" %d", ibits[i]);
    stack_bit (s, ibits[i]);
  }
  printf (" ]"
          "\nNumber of ON bits: %d. Array len %d : "
          "\nParsing bit stack      [", s->nentries, s->len);
  uint64_t * bits = (uint64_t *) s->stack, i, j;
  for (int k = 0; k < s->len >> 3; ++k)
  {
    int base = k << 6;
    i = bits[k];
    #if defined(__clang__) || defined(__GNUC__)
    while (i) {
      int bit = __builtin_ctzll(i);
      printf( " %d", base | bit);
      i &= (i - 1);  // clear lowest set bit
    }
    #else
    int l = 0;
    while (i) {
      if ( i & (uint64_t) 0x1 )
        printf( " %d", base | l);
      i>>=1; ++l;
    }
    #endif
  }
  printf (" ]");

}
