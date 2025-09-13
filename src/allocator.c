#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "allocator.h"
#include "bits.h"

/*
.. fixme : assumes sizeof(char) == 1
*/

typedef struct Block {
  struct Block * next, * prev;
  size_t size, allocsize;
  char * head;
} Block ;

static Block  * blockhead = NULL;
static void  ** bins      = NULL;
static unsigned bits      = 0;

static Block * block_new ( void ) {
  size_t s = BLOCK_SIZE;
  void * mem = NULL;
  while ( !(mem = malloc (s)) && (s > 2 * PAGE_SIZE) ) {
    s = s >> 1;
  }
  assert (mem);
  Block * block = (Block *) mem;
  /* fixme : may use "max_align_t" for safer alignment*/
  size_t bsize = (sizeof (Block) + 7) & ~((size_t) 7);
  *block = (Block) {
    .next = blockhead,
    .prev = NULL,
    .size = s - bsize,
    .allocsize = s,
    .head = (char *) mem + bsize
  };
  if (!bins) {
    bins = (void **) block->head;
    size_t binsize = 16* sizeof (void *);
    block->head += binsize;
    block->size -= binsize;
    memset (bins, 0, binsize);
  }
  return (blockhead = block);
}

static int block_available (size_t s) {
  assert ( s <= PAGE_SIZE );
  return ( blockhead && ( blockhead->size > s ) );
}

void destroy () {
  while ( blockhead ){
    Block * next = blockhead->next;
    free ( blockhead );
    blockhead = next;
  }
}

void * allocate ( size_t size ) {
  size = (size + 7) & ~((size_t) 7);
  if (!block_available (size)) {
    Block * oldb = blockhead, * newb = block_new();
    if (oldb) { oldb->prev = newb; }
  }
  char * mem = blockhead->head;
  memset (mem, 0, size);
  blockhead->head += size;
  blockhead->size -= size;
  return (void *) mem;
}

char * allocate_str ( const char * s ) {
  size_t size = strlen (s) + 1;
  char * str  = allocate (size);
  memcpy (str, s, size);
  return str;
}

/*
.. allocation, deallocation and reallocation from/to bins
*/

#if 0
#define ULL unsigned long long

static inline int floorlog2 ( ULL k ) {
  /* ⌊ log2 (k) ⌋ */
  return ULL_BITS - __builtin_clz (k);
}

static inline int ceillog2 ( ULL k ) {
  /* ⌈ log2 (k) ⌉ */
  return 1ull + ULL_BITS - __builtin_clz (k - 1ull);
}

typedef struct Chunk {
  struct Chunk *next;
  size_t len;
} Chunk;

#undef ULL
#endif

void deallocate ( void * m, size_t olds ) {
  /*
  .. fixme : recover old memory
  .. Dummy as of now. No recovery, yet.
  */
}

void * reallocate ( void * m, size_t olds, size_t s ) {
  if ( olds >= s ) {
    //memset ( (uint8_t *)m + s, 0, olds - s);
    return m;                                      /* Not expanding */
  }
  void * t = allocate (s);              /* New, larger memory chunk */
  memcpy (t, m, olds);
  deallocate (m, olds);
  return t;
}
