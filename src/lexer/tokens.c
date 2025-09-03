#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "hash.h"
#include "stack.h"
#include "allocator.h"

  /*
  .. Hashtable for { "rgx", "token name"} pair
  */
  typedef RgxList {
    struct RgxList * next;
    char * rgx;
  } RgxList;


  typedef struct Key {
    struct Key * next;
    char       * token;
    RgxList    * rgxlist;
    uint32_t     hash;
  } Key;

  /*
  .. Hashtable, resizes automatically when, the number of 
  .. inserted keys, 'n', exceeds 0.5 hsize.
  */
  typedef struct Table {
    int hsize, n;
    Key ** table;
  };
  
  Key ** hash_table  ( int n );
  Key * hash_lookup ( Table * t, char * token );
  int   hash_insert ( Table * t, char * token, char * rgx, int i ); 

/*
.. Murmurhash 3
*/
static inline 
uint32_t hash ( const char * key, uint32_t len ) {
  	
  /* 
  .. Seed is simply set as 0
  */
  uint32_t h = 0;
  uint32_t k;

  /*
  .. Dividing into blocks of 4 characters.
  */
  for (size_t i = len >> 2; i; --i) {
    memcpy(&k, key, sizeof(uint32_t));
    key += sizeof(uint32_t);

    /*
    .. Scrambling each block 
    */
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;

    h ^= k;
    h = ((h << 13) | (h >> 19)) * 5 + 0xe6546b64;
  }

  /*
  .. tail characters.
  .. This is for little-endian.  You can find a different version 
  .. which is endian insensitive.
  */  
  k = 0; 
  switch (len & 3) {
    case 3: 
      k ^= key[2] << 16;
      #if defined(__GNUC__)  &&  (__GNUC__ >= 7)
        __attribute__((fallthrough));
      #endif
    case 2: 
      k ^= key[1] << 8;
      #if defined(__GNUC__)  &&  (__GNUC__ >= 7)
        __attribute__((fallthrough));
      #endif
    case 1: 
      k ^= key[0];
      k *= 0xcc9e2d51;
      k = (k << 15) | (k >> 17);
      k *= 0x1b873593;
      h ^= k;
  }

  /* 
  .. Finalize
  */
  h ^= len;
  h ^= (h >> 16);
  h *= 0x85ebca6b;
  h ^= (h >> 13);
  h *= 0xc2b2ae35;
  h ^= (h >> 16);

  /* return murmur hash */
  return h;
}

void hash_table_free( Table * t) {
  if(!t) return;
  deallocate ( t->table );
  deallocate ( t );
}

static int hash_table_resize (Table * t) {
  
  t->table = reallocate (t->table, 
    t->hsize * sizeof (Key *), 2* t->hsize * sizeof (Key *) );
  t->hsize *= 2;

  #ifndef _RGX_VERBOSE_
    fprintf(stderr, "\nDoubling hash table size");
  #endif

  return 0;
}

static int iaction = 0;
static Stack * actions = NULL;

table_init{
  iaction = 0;
  actions = stack_new (0);
}

Key * hash_lookup ( Table * t, char * action ) {
  uint32_t h = hash (action, strlen (action));
  Key ** ptr = &t->table[h % t->hsize], * k;
  while ( (k = *ptr) != NULL ) {
    if (k->hash == hash && ! strcmp (action, k->action) ) return ptr;
    ptr = &k->next;
  }
  k         = allocate (sizeof(Key));
  k->action = allocate_str (action);
  k->hash   = hash;
  stack_push ( &k->rgxlist );
  return (*ptr = k);
}

int hash_insert ( Table * t, char * action, char * rgx ) {
  Key * k = hash_lookup (t, action);
  RgxList ** ptr = &k->rgxlist, * r;
  while ( (r = *ptr) != NULL ) {
    if ( !strcmp (r->rgx, rgx) ) 
      return RGXERR;                     /* { rgx, action } exists!! */
    ptr = &r->next;
  }
  *ptr = r = allocate (sizeof(RgxList));
  r->rgx   = allocate_str (rgx);
  return 0;
}
