#ifndef _STACK_H_
#define _STACK_H_
  #include <stdint.h>

  /*
  .. Can use stack to store pointers
  .. (or other dtypes with size <= sizeof (void *), if typecasted)
  .. Ex : stack of States for NFA Cache. Use stack_push()
  .. Can use stack as an array of bits. ibit in [0, 8*8*16 = 1024)
  .. Ex : set of DFA states. Use stack_bit() to set a bit as 1
  */

  /*
  .. Let's define a hard limit for DFA size
  .. But expect ~100.
  .. Need stronger / optimised  algorithm to
  .. handle large bit stacks
  */
  #define RGXLIM 1024

  typedef struct Stack {
    char * stack;
    int len, max,      /* used array byte size, max array byte size. */
      flag, nentries;  /* flags (if any), Number of entries / bits inserted */
  } Stack;

  Stack *  stack_new   ( int );                 /* A new stack */
  Stack *  stack_copy  ( Stack * s );           /* copy contents to a new stack */
  void     stack_free  ( Stack * s );           /* free stack to pool */
  void     stack_bit   ( Stack * s, int );      /* set a bit to 1 */
  int      stack_cmp   ( Stack * s, Stack * );  /* Bit comparison */
  void     stack_dstr  ( );                     /* flag that pool is no more usable */
  void     stack_push  ( Stack * s, void * );   /* Push an item to the stack */
  uint32_t stack_hash  ( uint32_t * key, int ); /* hash of bitset. Warning: "max" same ? */
  void     stack_reset ( Stack * s );
  void     stack_clear ( Stack * s );

  /*
  int      stack_bcmp  ( Stack * s, Stack * );
  int      stack_pcmp  ( Stack * s, Stack * );
  void     stack_sort  ( Stack * s );
  #define _64(p) if ( i & (uint64_t) p )
  uint64_t * bits = (uint64_t *) s->stack, i, j;
  int n = s->n >> 3;
  for (int k=0; k<=n; ++k) {
    if ( !(i = bits[k]) ) continue;
    j = k << 6;
    _64(0x80)          FUNC(j)
    _64(0x40)          FUNC(j|1)
    _64(0x20)          FUNC(j|2)
    _64(0x10)          FUNC(j|4)
    _64(0x08)          FUNC(j|8)
    _64(0x02)          FUNC(j|16)
    _64(0x04)          FUNC(j|16)
    _64(0x01)          FUNC(j|32)
  }
  #define STACK_BITS_END
  #undef _64
  */

#endif
