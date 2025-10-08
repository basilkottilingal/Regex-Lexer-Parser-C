
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

static char * lxr_eof = NULL;
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
static char lxr_dummy[3] = { '\n', '\0', '\0' };
static char * lxr_buff = lxr_dummy;                 /* current buffer */
static char * lxr_start = lxr_dummy + 1;        /* start of this token */
static char * lxr_bptr = lxr_dummy + 1;       /* buffer read location */
static size_t lxr_size = 1;                     /* current buff size */

/*
.. If user hasn't given a character input function, lxr_input() is
.. taken as default input function. User may override this by defining
.. a macro LXR_INPUT which calls a function with signature
..   int lxr_input ( void );
*/
#ifndef LXR_INPUT
  #define LXR_INPUT

  #ifndef LXR_BUFF_SIZE
    #define LXR_BUFF_SIZE  1<<16     /* default buffer size : 16 kB */
  #endif

  /*
  .. Creata a new buffer / expand buffer and fill it (or until EOF)
  */
  static void lxr_buffer_update () {
    /*
    .. Create a new buffer as we hit the end of the current buffer.
    */
    if (lxr_in == NULL) lxr_in = stdin;

    size_t non_parsed = lxr_bptr - lxr_start;
    /*
    .. "non_parsed" : Bytes already consumed by automaton but yet
    .. to be accepted. These bytes will be copied to the new buffer.
    .. fixme : may reallocate if non_parsed is > 50 % of buffer.
    .. (as of now, reallocate if non_parsed == 100% of buffer)
    */
    char * mem;
    if ( lxr_start == lxr_buff ) {
      /*
      .. Current buffer not sufficient for the token being read.
      .. Buffer size is doubled
      */
      lxr_size *=2;
      mem  = lxr_realloc ( lxr_buff - sizeof (void *),
        lxr_size + 2 + sizeof (void *) );
    }
    else {
      /*
      .. We add the buffer to the linked list of buffers, so at the
      .. end of the lexing pgm, you can remove each blocks.
      */
      lxr_size = (size_t) LXR_BUFF_SIZE;
      mem  = lxr_alloc ( lxr_size + 2 + sizeof (void *) );
      *( (char **) mem ) = lxr_buff;
      memcpy (mem + sizeof (void *), lxr_start, non_parsed);
    }

    if (mem == NULL) {
      fprintf (stderr, "LXR : ALLOC/REALLOC failed. Out of memory");
      exit (-1);
    }

    /*
    .. Update the pointers to the current buffer, last accepted byte
    .. and current reading ptr
    */
    lxr_buff = mem + sizeof (void *);
    lxr_start = lxr_buff;
    lxr_bptr = lxr_buff + non_parsed;

    /*
    .. Read from input file which the buffer can hold, or till the
    .. EOF is encountered
    */
    size_t bytes = non_parsed +
      fread ( lxr_bptr, 1, lxr_size - non_parsed, lxr_in );
    if (bytes < lxr_size) {
      if (feof (lxr_in)) lxr_eof = & lxr_buff [bytes];
      else {
        fprintf (stderr, "LXR : fread failed !!");
        exit (-1);
      }
    }
    lxr_buff [bytes] = lxr_buff [bytes+1] = '\0';
  }

  /*
  .. The default input function that outputs byte by byte, each in the
  .. range [0x00, 0xFF]. Exception : EOF (-1).
  */
  int lxr_input () {

    /*
    .. In case next character is unknown, you have to expand/renew the
    .. the buffer. It's because you need to look ahead to look for
    .. boundary assertion patterns like abc$
    */
    if ( lxr_bptr[1] == '\0' && lxr_eof == NULL )
      lxr_buffer_update ();

    /*
    .. return EOF without consuming, so you can call lxr_input () any
    .. number of times, each time returning EOF. 
    */
    if (lxr_bptr == lxr_eof)
      return EOF;

    /*
    .. A character in [0x00, 0xFF]
    */
    return (int) ( (unsigned char) *lxr_bptr++ );
      
  }

#endif

void lxr_clean () {
  /*
  .. Free all the memory blocks created for buffer.
  .. User required to run this at the end of the program
  */
  while ( lxr_buff != lxr_dummy ) {
    char * mem = lxr_buff - sizeof (char *);
    lxr_buff = * (char **) mem ;
    lxr_free (mem);
  }
}
