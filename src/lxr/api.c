

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
.. The list of api defined in lxr
.. (a) lxr_source() set the source file before starting the lexing.
..     If lxr_infile is not set by lxr_source(), assumes stdin as the
..     source of .lex grammar.
.. (b) lxr_input () return the next byte from the input source and
..     move the reading pointer.
..     Return value in [0x00, 0xFF] or EOF
.. (c) lxr_unput () return previous byte in [0x00, 0xFF].
..     Limit : You can unput a maximum of yyleng characters (and the
..     recently emulated lxr_input () characters).
..     Returns EOF, in case the pointer reaches yytext.
.. (d) lxr_clean() : clean the stack of buffers.
..     Call at the end of the program
*/

void lxr_source     ( const char * source );
void lxr_read_bytes ( const unsigned char *, size_t len );
int  lxr_input      ( );
int  lxr_unput      ( );
void lxr_clean      ( );
void lxr_stack_push ( const char * source );
void lxr_stack_pop  ( int isdestroy );

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

/*
..
*/
#ifndef YYSTYPE
  #define YYSTYPE  int lxr_lex ()
#endif

/*
.. Input stream of bytes on which tokenization happens. If no file or
.. bytes are specified, stdin is assumed as the source of bytes.
.. You can specify a file as input source using "lxr_source ()" or
.. a bytes array as the input source using "lxr_read_bytes ()"
*/
#define lxr_source_is_stdin 0
#define lxr_source_is_file  1
#define lxr_source_is_bytes 2
static int lxr_source_type    = lxr_source_is_stdin;
static char * lxr_infile      = NULL;
static FILE * lxr_in          = NULL;
static char * lxr_bytes_start = NULL;
static char * lxr_bytes_ptr   = NULL;

static void lxr_buffer_update ();

/*
.. yytext and yyleng are respectively the token text and token length
.. of the last accepted token
*/
static char lxr_sample[] = "\ndummy";
char * yytext = lxr_sample + 1;
int    yyleng = 0;

/*
.. Current line number and columen number.
*/
int lxr_line_no = 1;
int lxr_col_no = 1;
