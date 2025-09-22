/*
.. ----------------------- Lexer ------------------------------------ 
.. This is a generated file. This file contains functions and macros 
.. related to reading source code and running action when a lexicon
.. detected.
*/

#define LXR_BOUNDARY_BOF 1
#define LXR_BOUNDARY_BOL 2
#define LXR_BOUNDARY_EOF 4
#define LXR_BOUNDARY_EOL 8

static int lxrstatus = 0;     /* Encode read status & boundary info */
static FILE * lxrin  = stdin;
static FILE * lxrout = stdout;

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

  static char * lxrbuff = NULL;                   /* current buffer */
  static char * lxrlast = NULL;           /* last accepted location */
  static char * lxlbloc = NULL;             /* buffer read location */
  static size_t lxrsize = (size_t) LXR_BUFF_SIZE;

  /*
  .. The default input function that outputs byte by byte, each in the
  .. range [0x00, 0xFF]. Exception : EOF (-1).
  ..
  */
  int lxr_input () {
    if (!source) {
      char * mem  = LXR_ALLOC ( lxrsize + 2 + sizeof (void *));
      *( (void **) mem ) = NULL;  /* Chaining buffer blocks to free */
      source = mem + sizeof (void *);
      size_t bytes = fread ( source, 1, LXR_BUFF_SIZE, lxrin );
      if (bytes == 0 && !feof (lxr_in)) return LXR_ERROR;
      lxrbuff [bytes] = lxrbuff [bytes+1] = '\0';
      lxrlast = lxrbloc = lxrbuff;
    }
    if ( *lxrbloc = '\0' && lxrbloc[1] == '\0' &&
      lxrbloc
        
  }
  
#endif

void clean () {
  #ifdef LXR_FREE
    
  #endif
}

