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

#define LXRERR -2

static int lxrEOFyettoread = 1;
static int lxrprevchar = '\n';          /* encode '\n' for '^' bdry */
static int lxrBGNstatus = 1;
static int lxrENDstatus = 0;
static FILE * lxrin;

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

static char lxrdummy[3] = {'\0', 'x', '\0'};
static char * lxrbuff = lxrdummy;                 /* current buffer */
static char * lxrlast = lxrdummy + 1;     /* last accepted location */
static char * lxrbptr = lxrdummy + 1;       /* buffer read location */
static size_t lxrsize = 0;                     /* current buff size */

#define LXR_BOL() (lxrprevchar == '\n')
#define LXR_EOL() (*lxrbptr  == '\0' || *lxrbptr == '\n')
#define LXR_BDRY() do {                                              \
  lxrBGNstatus = LXR_BOL(); lxrENDstatus = LXR_EOL();                \
                   } while (0)


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
  .. Expand buffer and fill it (or until EOF)
  */
  static void lxr_buffer_update () {
    /*
    .. Create a new buffer as we hit the end of the current buffer.
    */
    if (!lxrin) lxrin = stdin;

    size_t non_parsed = lxrbptr - lxrlast;
    /*
    .. "non_parsed" : Bytes already consumed by automaton but yet
    .. to be accepted. These bytes will be copied to the new buffer.
    .. fixme : may reallocate if non_parsed is > 50 % of buffer.
    .. (as of now, reallocate if non_parsed == 100% of buffer)
    */
    char * mem;
    if ( lxrlast == lxrbuff ) {
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
      memcpy (mem + sizeof (void *), lxrlast, non_parsed);
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
    lxrlast = lxrbuff;
    lxrbptr = lxrbuff + non_parsed;

    /*
    .. Read from input file which the buffer can hold, or till the
    .. EOF is encountered
    */
    size_t bytes = non_parsed +
      fread ( lxrbptr, 1, lxrsize - non_parsed, lxrin );
    if (bytes < lxrsize) {
      if (feof (lxrin)) lxrEOFyettoread = 0;
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
    .. Note : lxrBGNstatus, lxrENDstatus are available even before
    .. lxr_input (). Thus you can get boundary information without
    .. consuming.
    */

    /*
    .. In case next character is unknown, you have to expand/renew the
    .. the buffer
    */
    if ( lxrbptr[1] == '\0' && lxrEOFyettoread )
      lxr_buffer_update ();

    /*
    .. report EOF without consuming, so you can lxr_input () any
    .. number of times, each time returning EOF.
    */
    if (lxrbptr [0] == '\0')
      return EOF;

    LXR_BDRY ();

    return (int) (lxrprevchar = *lxrbptr++);
  }

#endif

void lxr_clean () {
  /*
  .. Free all the memory blocks created for buffer
  */
  while ( lxrbuff != lxrdummy ) {
    char * mem = lxrbuff - sizeof (char *);
    lxrbuff = * (char **) mem ;
    LXR_FREE (mem);
  }
}
