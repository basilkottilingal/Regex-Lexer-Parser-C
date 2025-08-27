#ifndef _H_ALLOCATE_
#define _H_ALLOCATE_
  /*
  .. A general pool that allocates memory with size in (0, 4088 Bytes].
  .. Warning. There is no memory freeing (except at the end of pgm).
  .. So repeated allocations may exhaust RAM.
  .. May add a page deallocator so you recover pages (also allocate in pages).
  */

  #include <stdlib.h>

  #ifndef BLOCK_SIZE
    #define BLOCK_SIZE (1<<15)    /* 32 kB     */
  #endif
  #ifndef PAGE_SIZE
    #define PAGE_SIZE 4096
  #endif

  /* APIs
  .. (a) "deallocate" all mem blocks. Call @ the end of pgm
  .. (b) "allocate" memory. max 4088 Bytes. (Rounded to 8 bytes. Maybe unnecessary)
  .. (c) "allocate_str" : equivalent to strdup ()
  .. (d) allocate_stack(): sets a checkpoint so every allocate() after that can be
  ..     freed just by allocate_unstack().
  ..     Warning!! reusing any of the allocate() pointers b/w push & pop
  ..     may result in unexpected behaviour
  .. (e) allocate_unstack() gives back all memory allocated after the last
  ..     allocate_stack(). Warning!! missed pop() may waste memory.
  */
  void   deallocate ();
  void * allocate ( size_t size );
  char * allocate_str ( const char * s );
  void   allocate_stack ();
  void   allocate_unstack ();
#endif
