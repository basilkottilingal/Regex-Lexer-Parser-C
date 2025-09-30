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

static char * lxrEOF = NULL;
static FILE * lxrin = NULL;

extern void lxrin_set (FILE *fp) {
  if (fp == NULL) {
    fprintf (stderr, "Invalid lxrin");
    fflush (stderr);
    exit (-1);
  }
  lxrin = fp;
}

/*
.. If user hasn't defined alternative to malloc, realloc & free.
.. Note : In case user, defines alternative macros, please make sure
.. to use the signature of the malloc/realloc/free functions in stdlib
*/
#ifndef LXR_ALLOC
  #define LXR_ALLOC(_s_)        malloc  (_s_)
  #define LXR_REALLOC(_a_,_s_)  realloc (_a_, _s_)
  #define LXR_FREE(_a_)         free (_a_)
#endif
static char lxrdummy[3] = {0};
static char * lxrbuff = lxrdummy;                 /* current buffer */
static char * lxrstrt = lxrdummy + 1;        /* start of this token */
static char * lxrbptr = lxrdummy + 1;       /* buffer read location */
tatic size_t lxrsize = 1;                      /* current buff size */

/*
.. If user hasn't given a character input function, lxr_input() is
.. taken as default input function. User may override this by defining
.. a macro LXR_INPUT which calls a function with signature
..   int lxr_input ( void );
*/
#ifndef LXR_INPUT
  #define LXR_INPUT lxr_input ()

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
    if (lxrin == NULL) lxrin = stdin;

    size_t non_parsed = lxrbptr - lxrstrt;
    /*
    .. "non_parsed" : Bytes already consumed by automaton but yet
    .. to be accepted. These bytes will be copied to the new buffer.
    .. fixme : may reallocate if non_parsed is > 50 % of buffer.
    .. (as of now, reallocate if non_parsed == 100% of buffer)
    */
    char * mem;
    if ( lxrstrt == lxrbuff ) {
      /*
      .. Current buffer not sufficient for the token being read.
      .. Buffer size is doubled
      */
      lxrsize *=2;
      mem  = LXR_REALLOC ( lxrbuff - sizeof (void *),
        lxrsize + 2 + sizeof (void *) );
    }
    else {
      /*
      .. We add the buffer to the linked list of buffers, so at the
      .. end of the lexing pgm, you can remove each blocks.
      */
      lxrsize = (size_t) LXR_BUFF_SIZE;
      mem  = LXR_ALLOC ( lxrsize + 2 + sizeof (void *) );
      *( (char **) mem ) = lxrbuff;
      memcpy (mem + sizeof (void *), lxrstrt, non_parsed);
    }

    if (mem == NULL) {
      fprintf (stderr, "LXR : ALLOC/REALLOC failed. Out of memory");
      exit (-1);
    }

    /*
    .. Update the pointers to the current buffer, last accepted byte
    .. and current reading ptr
    */
    lxrbuff = mem + sizeof (void *);
    lxrstrt = lxrbuff;
    lxrbptr = lxrbuff + non_parsed;

    /*
    .. Read from input file which the buffer can hold, or till the
    .. EOF is encountered
    */
    size_t bytes = non_parsed +
      fread ( lxrbptr, 1, lxrsize - non_parsed, lxrin );
    if (bytes < lxrsize) {
      if (feof (lxrin)) lxrEOF = & lxrbuff [bytes];
      else {
        fprintf (stderr, "LXR : fread failed !!");
        exit (-1);
      }
    }
    lxrbuff [bytes] = lxrbuff [bytes+1] = '\0';
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
    if ( lxrbptr[1] == '\0' && lxrEOF == NULL )
      lxr_buffer_update ();

    /*
    .. return EOF without consuming, so you can call lxr_input () any
    .. number of times, each time returning EOF. 
    */
    if (lxrbptr == lxrEOF)
      return EOF;

    /*
    .. A character in [0x00, 0xFF]
    */
    if (*lxrbptr)
      return (int) ( (unsigned char) *lxrbptr++ );
      
  }

#endif

void lxr_clean () {
  /*
  .. Free all the memory blocks created for buffer.
  .. User required to run this at the end of the program
  */
  while ( lxrbuff != lxrdummy ) {
    char * mem = lxrbuff - sizeof (char *);
    lxrbuff = * (char **) mem ;
    LXR_FREE (mem);
  }
}
