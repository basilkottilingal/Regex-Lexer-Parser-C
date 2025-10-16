
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

static char lxr_sample[] = "\ndummy";
static unsigned char
  lxr_dummy[3] = {lxr_eob_class, lxr_eob_class, lxr_eob_class};
static unsigned char * lxr_start = lxr_dummy;
static unsigned char * lxr_bptr  = lxr_dummy;

static char lxr_hold_char = '\0';
char * yytext = lxr_sample + 1;
int    yyleng = 0;

static lxr_stack * lxr_current = NULL;
  
#ifndef LXR_BUFF_SIZE
  #define LXR_BUFF_SIZE  1<<16     /* default buffer size : 16 kB */
#endif
static size_t lxr_size = LXR_BUFF_SIZE;

/*
.. The default input function that outputs byte by byte, each in the
.. range [0x00, 0xFF]. Exception : EOF (-1).
*/
int lxr_input ();
/*
.. The default unput function that outputs byte by byte, each in the
.. range [0x00, 0xFF]. Exception : Error (-1). Limit : you can unput
.. until the beginning of the last token.
*/
int lxr_unput ();
/*
.. In case input() or unput () was called after accepting the last
.. token, create a new token depending on the pointer
*/
void lxr_token () {
  if ( yyleng == (int) (lxr_bptr - lxr_start) )
    return;
  yytext [yyleng] = lxr_hold_char;
  yyleng = (int) (lxr_bptr - lxr_start);
  lxr_hold_char = yytext [yyleng];
  yytext [yyleng] = '\0';
}

static void lxr_buffer_update ();
void lxr_clean () {
  /*
  .. Free all the memory blocks created for buffer.
  .. User required to run this at the end of the program
  */
  lxr_stack * s = lxr_current, * next;
  while (s != NULL) {
    next = s->next;
    free (s->bytes);
    free (s->class);
    free (s);
    s = next;
  }
}
