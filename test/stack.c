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
  Stack * p = stack_new (0);
  int val, * a = &val; val = 871;
  for (int i=0; i<25; ++i) stack_push (p,a);
  stack_free (s);
  int nbits = 199, bytes = (nbits + 63 & ~ (int) 63)>>3;
  s = stack_copy (p);
  Stack * B = stack_new (bytes);
  for (int i=0; i<nbits; i+=64)  {
    stack_bit (B, i);
    stack_bit (B, i+32);
  }
  void ** mem = (void **) s->stack;
  printf("\n");
  for (int i=0; i<s->len/sizeof (void *); ++i)
    printf("%d ", *(int *) mem[i]);
    
  printf("\n");
  uint32_t * _b32 = (uint32_t *) B->stack;
  for(int i=0; i< bytes>>2; ++i)
    printf("%u ", _b32[i]);
  
  printf("\n");
  uint64_t * _b64 = (uint64_t *) B->stack;
  for(int i=0; i< bytes>>3; ++i)
    printf("%lu ", _b64[i]);
}
