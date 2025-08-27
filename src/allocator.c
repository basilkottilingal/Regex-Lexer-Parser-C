#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "allocator.h"

typedef struct Block {
  struct Block * next, * prev;
  size_t size, allocsize;
  char * head;
} Block ;

Block * BlockHead = NULL, * FreeHead = NULL;
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
    .next = BlockHead,
    .prev = NULL,
    .size = s - bsize,
    .allocsize = s,
    .head = (char *) mem + bsize
  };
  return (BlockHead = block);
}

static int block_available (size_t s) {
  assert ( s <= PAGE_SIZE );
  return ( BlockHead && ( BlockHead->size > s ) );
}

void deallocate () {
  while ( BlockHead ){
    Block * next = BlockHead->next;
    free ( BlockHead );
    BlockHead = next;
  }
}

void * allocate ( size_t size ) {
  size = (size + 7) & ~((size_t) 7);
  if (!block_available (size)) {
    Block * oldb = BlockHead, * newb = block_new();
    if (oldb) { oldb->prev = newb; }
  }
  char * mem = BlockHead->head;
  memset (mem, 0, size);
  BlockHead->head += size;
  BlockHead->size -= size;
  return (void *) mem;
}

char * allocate_str ( const char * s ) {
  size_t size = strlen (s) + 1;
  char * str  = allocate (size);
  memcpy (str, s, size);
  return str;
}

void allocate_stack () {
  /* fixme  */
}

void allocate_unstack () {
  /* fixme  */
}
