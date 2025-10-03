/*
.. Read a lex file. Follows the flex's ".lex" format. You can refer to
.. the flex documentation on the input file format here
.. https://westes.github.io/flex/manual/Format.html#Format
.. Input file need to follow the format
..
.. definitions
.. %%
.. rules
.. %%
.. user code
..
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"
#include "allocator.h"
#include "lexer.h"
#include "regex.h"

/*
.. Flex macro that holds a regex pattern. Macro naming should follow
.. the format [_a-zA-Z][_a-zA-Z0-9]*. You can recurse macro, i.e a
.. macro can use a previous macro, in it's definition. Macros are
.. defined in the definition section. Examples of macros are below
.. 
.. ALPH   [_a-zA-Z]
.. DIGITS [0-9]
.. ID     {ALPHA}({ALPH}|{DIGITS})*
*/

typedef struct Macro {
  char * key, * rgx;
  int flag;
  struct Macro * next;
} Macro;

#define prime 67u
static Macro * table [prime];

/*
.. A code snippet is defined in the definitions section in either of
.. the two forms as given below
..
.. %top{
..   #include <stdio.h>
..   #include "func.h"
.. }
.. 
.. %{
..   #include <stdio.h>
..   #include "func.h"
.. %}
..
.. The difference is that %top{} snippets will be forcefully placed at
.. the top and the %{%} snippet at the bottom. So if multiple %top{}
.. snippets appear, the last one in the lex file will be forced on the
.. top of the generated lexer file.
*/

typedef struct Snippet {
  char * code;
  struct Snippet * next;
} Snippet;

static Snippet * snippets = NULL;

/*
.. patterns are defined exclusively in the rules section, that falls
.. in between two %% separators. The pattern should be followed by the
.. corresponding action. Pattern is defined in the POSIX extended
.. regular expressions (ERE) with few exceptions
.. (a) strings are allowed in quotes
.. (b) macros inside {}
.. (c) . (dot or "all-character" class) excludes '\n'
.. (d) anchors '^' and '$' are allowed only at the beginning/end of
..     the tokens respectively 
..
.. %%
.. ID       { return IDENTIFIER; }
.. %%
*/

typedef struct Pattern {
  char * pattern, * action;
  int id, flag;
} Pattern;

static Pattern * patterns = NULL;

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

static Macro ** lookup (const char * key) {
  uint32_t h = hash (key, strlen (macro));
  Macro ** p = & table [h % prime], * m;
  while ( (m = *p) != NULL ) {
    if ( !strcmp (m->key, key) )
      return p;
    p = & m->next;
  }
  return p;
}

static int macro_new (const char * macro, const char * pattern) {
  uint32_t h = hash (macro, strlen (macro));
  Macro ** p = lookup (macro), * m;
  if ( (m = *p) != NULL ) {
    error ( "macro %s redefined in lexer definition in line %d",
      macro, line);
    return RGXERR;
  }
  *p = m = allocate (sizeof (Macro));
  p->key = allocate_str (macro);

  char rgx [RGXSIZE];
  char * ptr = pattern, c;
  size_t len = 0;
  while ( (c = *ptr++) ) {

    char * strt = ptr - 1;

    if ( c != '{' || (c = *ptr++) == '\\' ) {
      if ( c == '\\'
      if ( len == RGXSIZE-1 ) {
        error ("rgx buffer out of memory"); 
        return RGXOOM;
      }
      rgx [len++] = c;
      continue;
    }

    if ( (c = *ptr++) == '\0' ) {
      error ("incomplete pattern");
      return RGXERR;
    }

    if ( c == ',' || ( c <= '9' && c >= '0' ) )
      
    if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')){
      char * child = ptr - 1, * holdloc = NULL, holdchar;
      while ( (c = *ptr++) ) {
        if ( c == '}' ) {
          *holdloc = ptr - 1;
          if (holdloc - child == 1) {
            error ("empty {}");
            return RGXERR;
          }
          holdchar = *holdloc;
          *holdloc = '\0';
          p = lookup (child);
          if ( (m = *p) == NULL ) {
            error ("macro {%s} not found", child);
            *holdloc = holdchar;
            return RGXERR;
          }
          size_t l = strlen (m->rgx);
          if (l + len >= RGXSIZE) {
            error ("rgx buffer out of memory"); 
            return RGXOOM;
          }
          memcpy ( & rgx [len], m->rgx, l );
          len += l; 
        }
        if ( !( c == '_' || (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) )
          continue;
        }
      }
    }
  } 

  int rpn [RGXSIZE];

  p->rgx = allocate_str (rgx);

  if ( rgx_rpn (rgx, rpn) < 0) {
    error ("cannot create rpn for regex %s in line %d", rgx, line);
    return RGXERR;
  }
  
  return 0;
}

static int line = 1;

static void definitions ( FILE * in ) {
  char buff [BUFSIZ];
  
}
