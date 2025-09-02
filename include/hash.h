#ifndef _HASH_H_
#define _HASH_H_

  typedef struct Key {
    struct Key * next;
    char       * key;
    void       * value;
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
  Key ** hash_lookup ( Table * t,   char * key );
  void   hash_insert ( Key ** slot, char * key, void * value ); 
#endif
