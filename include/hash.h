#ifndef HASH_SIZE
  #define HASH_SIZE 64
#endif

static int rule_id = 0;

typedef struct Rule {
  struct Rule * next;
  char * name;
  int id;
} Rule;

Rule * Rules [HASH_SIZE] = {0};

static
unsigned bucket_index (const char * str) {
  unsigned h = 5381;
  while ( *str )
    h = ((h << 5) + h) + (unsigned char)(*str++);
  return h % HASH_SIZE;
}

Rule * rule_new ( const char * name ) {
  unsigned idx = bucket_index ( name );
  Rule * r =  Rules [idx] ;
  while (r) { 
    assert ( strcmp(r->name, name) );
    r = r->next;
  }
  r = allocate ( sizeof (Rule) );
  *r = (Rule) { .id = rule_id++, .name = allocate_str (name), .next = Rules[idx] };
  return ( Rules[idx] = r );
}
