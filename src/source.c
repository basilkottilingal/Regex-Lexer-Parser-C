/*
.. ----------------------- Lexer ------------------------------------
.. This is a generated file. This file contains functions and macros
.. related to reading source code and running action when a lexicon
.. detected.
*/

#define LXRERR -2

#define LXR_BOUNDARY_BOL 2
#define LXR_BOUNDARY_EOF 4
#define LXR_BOUNDARY_EOL 8
#define LXR_FREAD_EOF    16

static int lxrstatus = LXR_BOUNDARY_BOL;
static FILE * lxrin  = stdin;
static FILE * lxrout = stdout;

extern lxrin_set (FILE *fp) {
  lxrin = fp;
}

extern lxrout_set (FILE *fp) {
  lxrout = fp;
}

/*
.. If user hasn't defined alternative to both malloc and realloc
*/
#ifndef LXR_ALLOC
  #define LXR_ALLOC(_s_)        malloc  (_s_)
  #define LXR_REALLOC(_a_,_s_)  realloc (_a_, _s_)
  #define LXR_FREE(_a_)         free (_a_)
#endif

/*
.. If user hasn't given a character input () function. User may
.. override this with defining a macro LXR_INPUT which calls a
.. function with signature
..   int lxr_input ( void );
*/
#ifndef LXR_INPUT
  #define LXR_INPUT lxr_input ()

  #ifndef LXR_BUFF_SIZE
    #define LXR_BUFF_SIZE  1<<16     /* default buffer size : 16 kB */
  #endif

  static char lxrdummy[3] = {0};
  static char * lxrbuff = lxrdummy;               /* current buffer */
  static char * lxrlast = lxrdummy + 1;   /* last accepted location */
  static char * lxrbptr = lxrdummy + 1;     /* buffer read location */
  static size_t lxrsize = 0;                   /* current buff size */

  /*
  .. The default input function that outputs byte by byte, each in the
  .. range [0x00, 0xFF]. Exception : EOF (-1).
  */
  int lxr_input () {
    if ( lxrbptr[0] == '\0' && lxrbptr[1] == '\0' &&
         (lxrstatus & LXR_FREAD_EOF == 0) ) {

      /*
      .. Create a new buffer as we hit the end of the current buffer.
      .. We add the buffer to the linked list of buffers, so at the
      .. pgm, you can remove each blocks.
      */
      size_t non_parsed = lxrbptr - lxrlast;
      if ( lxrlast == lxrbuff ) {
        /*
        .. Current buffer not sufficient for the token being read.
        .. Buffer size expanded by extra BUFF_SIZE
        */
        lxrsize *=2; //+= LXR_BUFF_SIZE;
        char * mem  = LXR_REALLOC ( lxrbuff - sizeof (void *),
          lxrsize + 2 + sizeof (void *) );
      }
      else {
        lxrsize = (size_t) LXR_BUFF_SIZE;
        char * mem  = LXR_ALLOC ( lxrsize + 2 + sizeof (void *) );
        *( (void **) mem ) = lxrbuff;
        memcpy (mem + sizeof (void *), lxrlast, non_parsed);
      }
      lxrbuff = mem + sizeof (void *);
      lxrlast = lxrbuff;
      lxrbptr = lxrbuff + non_parsed;
      size_t bytes = non_parsed + 
        fread ( lxrbptr, 1, lxrsize - non_parsed, lxrin );
      if (bytes < lxrsize) {
        if (feof (lxrin)) lxrstatus |= LXR_FREAD_EOF;
        else return LXR_ERROR;
      }
      lxrbuff [bytes] = lxrbuff [bytes+1] = '\0';
    }

    /*lxrstatus &= ~LXR_BOUNDARY_BOL;*/

    if (lxrbptr [0] == '\0')
      return EOF;

    if (lxrbptr [1] == '\0')
      lxrstatus  |= LXR_BOUNDARY_EOF;

    return *lxrbptr++;
  }

#endif

void clean () {
  #ifdef LXR_FREE

  #endif
}

