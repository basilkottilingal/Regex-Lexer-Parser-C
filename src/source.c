
/*
.. ---------------------------- Lexer --------------------------------
.. This is a generated file for table based lexical analysis. The file
.. contains (a) functions and macros related to reading source code
.. and running action when a lexicon detected (b) compressed table for
.. table based lexical analysis.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE * lxr_in = NULL;
extern void lxr_in_set (FILE *fp) {
  if (fp == NULL) {
    fprintf (stderr, "Invalid lxr_in");
    fflush (stderr);
    exit (-1);
  }
  lxr_in = fp;
}

/*
.. If user hasn't defined alternative to malloc, realloc & free.
.. Note : In case user, defines alternative macros, please make sure
.. to use the signature of the malloc/realloc/free functions in stdlib
*/
#ifndef LXR_ALLOC
  #define lxr_alloc(_s_)        malloc  (_s_)
  #define lxr_realloc(_a_,_s_)  realloc (_a_, _s_)
  #define lxr_free(_a_)         free (_a_)
#endif

typedef struct lxr_stack {
  char * bytes;
  unsigned char * class;
  size_t size;
  struct lxr_stack * next; 
} lxr_stack ;

static char lxr_sample[] = "\n";
static unsigned char lxr_dummy[] = {lxr_eob_class, lxr_eob_class};
static unsigned char * lxr_start = lxr_dummy;
static unsigned char * lxr_bptr  = lxr_dummy;

static char lxr_hold_char = '\0';
extern char * yytext = lxr_sample + 1;
extern int    yyleng = 0;

static lxr_stack * lxr_current = NULL;
  
#ifndef LXR_BUFF_SIZE
#define LXR_BUFF_SIZE  1<<16     /* default buffer size : 16 kB */
#endif
static size_t lxr_size = LXR_BUFF_SIZE;

static void lxr_buffer_update () {

  if (lxr_in == NULL) lxr_in = stdin;
    
  /*
  .. "non_parsed" : Bytes already consumed by automaton but yet
  .. to be accepted. These bytes will be copied to the new buffer.
  .. fixme : may reallocate if non_parsed is > 50 % of buffer.
  .. (as of now, reallocate if non_parsed == 100% of buffer)
  */
  size_t size = lxr_size, non_parsed = lxr_bptr - lxr_start;

  lxr_stack * s = lxr_current;
  if (s == NULL || (yytext - s->bytes) > (size_t) 1 ) {

    while ( size <= non_parsed )
      size *= 2;

    s = lxr_alloc (sizeof (size));
    if ( s == NULL ) {
      fprintf (stderr, "lxr_alloc failed");
      exit (-1);
    }
    * s = (lxr_stack) {
      .size  = size,
      .bytes = lxr_alloc (size + 2),
      .class = lxr_alloc (size + 2),
      .next  = lxr_current
    };
    if ( s->bytes == NULL || s->class == NULL ) {
      fprintf (stderr, "lxr_alloc failed");
      exit (-1);
    }

    memcpy (s->bytes, & yytext [-1], non_parsed + 1);
    memcpy (s->class, lxr_start,     non_parsed);

    lxr_current = s; 
  }
  else {
    size = s->size * 2;
    s->bytes = lxr_realloc (s->bytes, size + 2);
    s->class = lxr_realloc (s->class, size + 2);
    s->size  = size;
    if ( s->bytes == NULL || s->class == NULL ) {
      fprintf (stderr, "lxr_realloc failed");
      exit (-1);
    }
  }

  yytext = s->bytes + 1;
  lxr_start = s->class;
  lxr_bptr = lxr_start + non_parsed;
  lxr_hold_char = *lxr_start;
    
  size_t bytes =
    fread ( & yytext [non_parsed], 1, lxr_size - non_parsed, lxr_in );

  lxr_bptr [bytes] = lxr_eob_class;
  lxr_bptr [bytes + 1] = lxr_eob_class;
  yytext   [bytes + non_parsed] = '\0';
  if (bytes < lxr_size - non_parsed) {
    if (feof (lxr_in)) {
      lxr_bptr [bytes] = lxr_eof_class;
      lxr_bptr [bytes + 1] = lxr_eof_class;
    }
    else {
      fprintf (stderr, "lxr buffer : fread failed !!");
      exit (-1);
    }
  }

  unsigned char * byte = & ((unsigned char *) yytext) [non_parsed] ;
  for (size_t i=0; i<bytes; ++i)
    lxr_bptr [i] = lxr_class [ *byte++];
}
  
/*
.. The default input function that outputs byte by byte, each in the
.. range [0x00, 0xFF]. Exception : EOF (-1).
*/
int lxr_input () {
  if (*lxr_bptr == lxr_eob_class)
    lxr_buffer_update ();
  if (*lxr_bptr == lxr_eof_class)
    return EOF;
  return (int) (unsigned char) ( yytext [lxr_bptr++ - lxr_start] );
}

#endif

void lxr_clean () {
  /*
  .. Free all the memory blocks created for buffer.
  .. User required to run this at the end of the program
  while ( lxr_buff != lxr_dummy ) {
    char * mem = lxr_buff - sizeof (char *);
    lxr_buff = * (char **) mem ;
    lxr_free (mem);
  }
  */
}
