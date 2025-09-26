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

static char * lxrEOF = NULL;
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

static char lxrdummy[3] = {0};
static char * lxrbuff = lxrdummy;                 /* current buffer */
static char * lxrstrt = lxrdummy + 1;        /* start of this token */
static char * lxrbptr = lxrdummy + 1;       /* buffer read location */
static size_t lxrsize = 1;                     /* current buff size */

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
  .. Creata a new buffer / expand buffer and fill it (or until EOF)
  */
  static void lxr_buffer_update () {
    /*
    .. Create a new buffer as we hit the end of the current buffer.
    */
    if (!lxrin) lxrin = stdin;

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
    .. A character in (0x00, 0xFF]
    */
    if (*lxrbptr)
      return (int) (lxrprevchar = *lxrbptr++);
      
    /*
    .. return EOF without consuming, so you can call lxr_input () any
    .. number of times, each time returning EOF. 
    */
    if (lxrbptr == lxrEOF)
      return EOF;

    /* 
    .. Found an unexpected 0xff !! in the source file. It's behaviour
    .. with the automaton depends on the pattern. Eventhough 0xff are
    .. not expected in utf8 files, you may still encounter them if
    .. any corruption happened. 
    */
    return  
      (int) (lxrprevchar = *lxrbptr++);

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


static short check [1184] = {
   -1,

   -1,   51,  284,   -1,   -1,   -1,  284,  284,  117,   -1,
  116,  284,  112,  112,  112,  112,  112,  279,  112,  268,
  312,  106,   -1,   -1,   -1,  106,  112,  112,  116,  284,
  108,  284,  114,   -1,  316,  316,  316,  316,  316,  103,
  316,   87,   72,  284,   71,  287,  274,  284,  316,  316,
   94,   86,  302,  341,   98,  273,  284,  356,  354,  350,
  349,  284,  112,  112,  355,  357,  338,  350,  353,  284,
  279,   51,  268,  112,  351,  355,  112,  357,  112,   49,
  353,  112,  345,  334,  316,  316,  351,  331,  280,  345,
  280,  352,  280,  280,  348,  316,   49,  280,  316,  280,

  316,  352,  346,  316,  348,  341,  347,  344,  342,  343,
   49,  343,  346,  330,   49,  340,  335,  280,  347,  344,
  342,  335,  340,  280,  280,  280,  280,  339,   49,  337,
  337,  328,   49,  328,  339,  326,   49,  326,  280,  280,
  280,  280,  280,  113,  266,  113,  332,  113,  113,  336,
  336,  332,  113,  317,  113,  317,  333,  317,  317,  333,
  329,  325,  317,  329,  317,  330,  327,  324,  323,  327,
  252,  320,  113,  322,  325,  323,  320,  324,  113,  113,
  113,  113,  317,  322,  263,  263,  321,  254,  317,  317,
  317,  317,  258,  113,  113,  113,  113,  113,  321,  262,

  258,  259,  262,  317,  317,  317,  317,  317,  282,  265,
  282,  259,  282,  282,  264,  265,  261,  282,  264,  282,
  260,  257,  256,  261,  260,  255,  253,  257,  249,  251,
  255,  256,  251,  247,  253,  282,  250,  282,  248,  254,
  249,  246,  248,  282,  282,  282,  282,  250,  244,  246,
  241,  235,  233,  235,  223,  245,  231,  231,  282,  282,
  282,  282,  282,  358,  358,  245,  358,  358,  358,  358,
  243,  242,  240,  358,  358,  358,  239,  358,  238,  237,
  234,  236,  243,  242,  239,  240,  228,  232,  238,  236,
  234,  232,  237,  358,  228,  227,  202,  227,  220,  358,

  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  220,  193,  164,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  230,  229,  225,  226,
  224,  230,  229,  226,  222,  219,  225,  219,  222,  221,
  224,  218,  221,  217,  216,  215,  214,  216,  212,  218,
  217,  214,  215,  214,  213,  212,  211,  209,  213,  210,
  213,  214,  208,  210,  204,  208,  213,  203,  211,  209,
  207,  206,  207,  205,  201,  204,  206,  205,  203,  200,
  199,  198,  201,  197,  196,  194,  195,  200,  191,  198,

  195,  192,  192,  199,  190,  197,  189,  188,  194,  189,
  191,  187,  192,  192,  190,  186,  187,  185,  182,  187,
  188,  185,  184,  186,  184,  183,  181,  176,  182,  180,
  183,  179,  178,  177,  179,  177,  178,  161,  181,  176,
  180,  180,  175,  173,  174,  175,  196,  174,  173,  172,
  171,  170,  169,  159,  168,  167,  172,  168,  167,  165,
  169,  171,  166,  166,  170,  168,  165,  163,  160,  162,
  163,  158,  163,  162,  157,  160,  156,  155,  154,  158,
  157,  154,  153,  162,  152,  151,  156,  142,  150,  155,
  150,  149,  151,  145,  145,  138,  149,  152,  148,  148,

  148,  136,  148,  148,  147,  148,  148,  148,  146,  147,
  140,  146,  148,  135,  139,  141,  147,  148,  144,  130,
  144,  144,  141,  139,  137,  137,  140,  132,  133,  132,
  144,   47,  144,  133,  153,  131,  131,  129,  129,  128,
  127,  128,  126,  125,  127,  124,  123,  126,  122,  125,
  121,  120,  124,  119,  123,  118,  120,   46,  122,   45,
   44,  121,  115,  120,   43,  135,  115,   42,  119,   41,
   40,  118,  135,   39,  115,   38,   37,   36,   35,   34,
   33,   32,   31,   30,   29,   28,   27,   26,   25,   24,
   23,   22,   21,   20,   19,   18,   17,   16,   15,   14,

   13,   12,   11,   10,    9,    8,    7,    6,    5,    4,
    3,  143,  143,  319,  134,   48,   10,  275,  289,  303,
  300,  304,  134,   48,  100,  100,  276,  104,  143,  102,
  102,  272,  285,  290,   97,   93,  298,  134,  300,  286,
  271,  301,   97,  304,  288,   -1,  293,   93,  276,  272,
  285,   -1,   -1,   -1,   -1,  290,   99,   -1,   -1,   -1,
   99,  319,  134,   48,  293,   99,   -1,  104,   -1,  134,
  314,  314,  314,  314,  314,  314,  314,  314,  314,  314,
  314,   -1,  314,  314,  314,  314,  314,   -1,  314,  314,
  314,  314,  314,  314,  314,  314,  314,  314,  314,  314,

  314,  314,  314,  314,  314,  314,  314,  314,  314,  314,
  314,  314,  314,  314,  314,  314,  314,  314,  314,  314,
  314,  314,  314,  314,  314,  314,  314,  314,  314,  314,
  314,  314,  314,  314,  314,  314,  314,  314,  314,  314,
  314,  314,  314,  314,  314,  314,  314,  314,  314,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
   -1,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,

  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,    0,    0,
   -1,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   96,   -1,   -1,    0,   -1,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   96,   96,   -1,  308,
   -1,  299,    0,  283,  269,   -1,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  308,    0,  299,
    0,  283,  269,   -1,   -1,    0,    0,    0,    0,    0,

   -1,   -1,   -1,    0,    0,    0,    0,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  107,  270,  107,  306,
   -1,  270,  270,  306,  306,   -1,   -1,  101,  107,  107,

  107,  313,  107,   -1,   -1,  313,  313,  101,  310,  101,
  101,   -1,  310,  310,   -1,  294,  270,  281,  306,  294,
  294,  281,  281,   -1,  107,  107,  107,  107,  107,  107,
  313,  107,  107,  107,   92,   -1,   57,  310,   92,   92,
   -1,   53,   58,  309,  294,  107,  281,  107,   56,  107,
  107,  107,  107,   -1,   -1,   -1,   -1,   -1,  107,  107,
  107,   92,   59,   92,   59,  297,   -1,   55,   59,  297,
  297,   -1,   57,   -1,  297,   57,   -1,   53,   58,  309,
   53,   58,  309,   -1,   56,   -1,   54,   56,   -1,  296,
   54,   54,  297,   57,  297,   54,  296,   59,   53,   58,

  309,   59,   -1,   55,   50,   56,   55,  284,   50,   50,
   -1,  284,   -1,   -1,   -1,   54,   -1,   -1,   -1,  297,
   -1,   -1,   -1,   59,   55,  296,   -1,   -1,  296,   -1,
   -1,   -1,   -1,   50,   -1,   -1,   -1,   -1,   -1,   -1,
   54,  296,   -1,   -1,   -1,   50,  296,   -1,  284,   50,
   -1,   -1,  284,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   50,   -1,   -1,  284,   -1,   -1,   -1,
   -1,   50,   -1,   -1,  284,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1
};

static short next [1184] = {
    0,

    0,  282,  284,    0,    0,    0,  284,  297,  110,    0,
  270,  308,  110,  110,  110,  110,  110,  269,  110,  116,
   52,  106,    0,    0,    0,  106,  110,  110,  270,  296,
   60,  297,  117,    0,  312,  312,  312,  312,  312,   69,
  312,   80,   62,  286,   61,  291,  277,  285,  312,  312,
   81,   91,  305,   27,   65,  276,  308,    8,   11,  266,
   12,  288,  110,  110,  266,  266,  337,    3,  266,  285,
  269,  282,  116,  110,  266,  358,  110,    7,  110,  283,
    9,  113,  266,   19,  312,  312,  350,   35,  279,   14,
  279,  266,  279,  279,  266,  312,  268,  279,  312,  279,

  312,    6,  266,  317,   16,  266,  266,  266,  266,  344,
  273,  266,  345,   31,  272,  266,  266,  279,   15,   23,
   25,  354,   24,  279,  279,  279,  279,  266,  271,   26,
  266,   28,  283,  266,  338,   39,  272,  266,  279,  279,
  279,  279,  279,  110,  266,  110,  266,  110,  110,   33,
  266,  331,  110,  312,  110,  312,  266,  312,  312,  332,
  266,  266,  312,  328,  312,  266,  266,  266,  266,  326,
  119,  266,  110,  266,  324,   32,   40,  340,  110,  110,
  110,  110,  312,  342,  266,  262,  266,   38,  312,  312,
  312,  312,  266,  110,  110,  110,  110,  110,   29,  266,

  257,  266,  261,  312,  312,  312,  312,  312,   49,  266,
   49,  118,   49,   49,  263,  264,  266,   49,  266,   49,
  259,  266,  266,  260,  266,  266,  266,  256,  266,  266,
  252,  255,  250,  122,  341,  280,  266,   49,  352,  266,
  247,  266,  266,   49,   49,   49,   49,  249,  243,  245,
  346,  266,  232,  234,  222,  266,  230,  266,   49,   49,
   49,   49,   49,  266,  266,  244,  266,  266,  266,  266,
  266,  266,  266,  266,  266,  266,  266,  266,  266,  266,
  266,  266,  240,  241,  323,  238,  266,  231,  124,  225,
  233,  266,  125,  266,  227,  226,  182,  266,  266,  266,

  266,  266,  266,  266,  266,  266,  266,  266,  266,  266,
  219,  192,  336,  266,  266,  266,  266,  266,    5,  266,
  266,  266,  266,  266,  266,  266,  266,  266,  266,  266,
  266,  266,  266,  266,  266,  266,  229,  266,  266,  126,
  266,  266,  228,  266,  127,  128,  320,  266,  266,  266,
  223,  266,  220,  266,  266,  266,  266,  215,  266,  217,
  129,  171,  170,  237,  355,  180,  266,  266,  266,  209,
  248,  242,  266,  266,  266,  207,  212,  266,  210,  208,
  206,  205,  266,  204,  266,  203,  266,  266,  202,  266,
  266,  266,  322,  266,  195,  266,  194,  197,  266,   17,

  266,  266,  251,  351,  266,  198,  266,  266,  183,  343,
  190,  266,  191,  325,  189,  266,  186,  347,  266,  348,
  184,  266,  321,  185,  266,  266,  266,  266,  181,  266,
  353,  266,  176,  266,  172,  174,  266,  160,   45,  333,
  357,  216,  335,  266,  266,  266,  266,  173,  130,  266,
  266,  266,  266,  156,  266,  266,  334,  239,  166,  266,
  163,  349,  266,  165,  356,  167,  164,  266,  266,  155,
  131,  266,  178,  266,  266,  159,  266,  266,  266,  157,
  327,  132,  152,  188,  266,  266,  154,  253,  149,  329,
  266,  133,  150,  266,  141,  137,  266,  151,  147,  236,

  258,  330,  161,  265,  266,  246,  211,  235,  266,  146,
  266,  145,  153,   18,  266,  266,  158,  266,  266,   36,
  201,  143,  115,  138,  136,  266,  139,   42,   47,  266,
  162,  266,  221,  266,  266,   34,  266,  266,   21,  266,
  266,   30,  266,  266,    4,  266,  196,   46,  266,   13,
  266,  266,   44,  266,   10,  266,  179,  266,   22,  266,
  266,   37,  121,   20,  266,  266,  266,  266,   41,  266,
  266,   43,  120,  266,  254,  266,  266,  266,  266,  266,
  266,  266,  266,  266,  266,  266,  266,  266,  266,  266,
  266,  266,  266,  266,  266,  266,  266,  266,  266,  266,

  266,  266,  266,  266,  266,  266,  266,  266,  266,  266,
  266,  266,  339,  110,  110,  110,  175,  277,  291,  305,
  302,  305,  314,  314,   67,   84,  277,   70,  142,   79,
   71,  274,  287,  291,   73,   76,  304,  319,  303,  290,
  276,  304,   63,  305,  290,    0,  306,   68,  277,  275,
  289,    0,    0,    0,    0,  291,    1,    0,    0,    0,
  318,  266,  266,  266,  306,   66,    0,   77,    0,  168,
  312,  312,  312,  312,  316,  312,  312,  312,  312,  312,
  312,    0,  312,  312,  312,  312,  312,    0,  312,  312,
  312,  312,  312,  312,  312,  312,  312,  312,  312,  312,

  312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
  312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
  312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
  312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
  312,  312,  312,  312,  312,  312,  312,  312,  312,  110,
  110,  110,  110,  112,  110,  110,  110,  110,   59,  110,
    0,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,

  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  107,  315,
    0,   50,  315,  187,  169,   51,   50,  109,   96,  106,
   74,    0,    0,  106,    0,  111,  105,  315,  315,  315,
   94,  100,   93,   88,   89,   98,   97,   85,   92,   99,
   50,   86,   82,  101,   87,  102,   64,   75,    0,  313,
    0,  310,   48,  294,  281,    0,   48,   90,   91,  103,
  148,  199,  224,  213,  123,  214,  200,  313,  135,  310,
  218,  294,  281,    0,    0,  193,  144,  140,  134,  177,

    0,    0,    0,   83,  104,   84,   95,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,    2,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  315,   58,  266,   55,
    0,   58,   58,   55,   55,    0,    0,   83,  266,  266,

  266,   53,  266,    0,    0,   53,   53,   90,  309,   72,
   78,    0,  309,  309,    0,   56,   58,   57,   55,   56,
   56,   57,   57,    0,  266,  266,  266,  266,  266,  266,
   53,  266,  266,  266,   54,    0,  278,  309,   54,   54,
    0,  311,  267,  307,   56,  266,   57,  266,  292,  266,
  266,  266,  266,    0,    0,    0,    0,    0,  266,  266,
  266,  108,  110,   54,   59,  297,    0,  295,   59,  297,
  297,    0,  278,    0,  308,  278,    0,  311,  267,  307,
  311,  267,  307,    0,  292,    0,   54,  292,    0,  295,
   54,   54,  296,  278,  297,  299,  293,  117,  311,  267,

  307,  117,    0,  295,   50,  292,  295,  284,   50,   50,
    0,  284,    0,    0,    0,   54,    0,    0,    0,  308,
    0,    0,    0,  114,  295,  295,    0,    0,  295,    0,
    0,    0,    0,   50,    0,    0,    0,    0,    0,    0,
  299,  293,    0,    0,    0,  301,  295,    0,  286,  300,
    0,    0,  285,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  298,    0,    0,  288,    0,    0,    0,
    0,  300,    0,    0,  285,    0,    0,    0,    0,    0,
    0,    0,    0
};

static short base [359] = {
  829,

    0,    0,  554,  553,  552,  551,  550,  549,  548,  547,
  546,  545,  544,  543,  542,  541,  540,  539,  538,  537,
  536,  535,  534,  533,  532,  531,  530,  529,  528,  527,
  526,  525,  524,  523,  522,  521,  520,  519,  517,  514,
  513,  511,  508,  504,  503,  501,  475,  607,   67, 1102,
    0,    0, 1037, 1084, 1063, 1044, 1032, 1038, 1054,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    9,    7,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   15,    6,    0,    0,    0,
    0, 1032,  612,   15,    0,  831,  607,   19,  630,  589,

  975,  594,    4,  592,    0,   11,  987,    1,    0,  750,
    0,   10,  141,    1,  510,    1,    0,  499,  497,  495,
  494,  492,  490,  489,  487,  486,  484,  483,  481,  463,
  480,  473,  477,  606,  509,  445,  469,  439,  458,  454,
  459,  431,  555,  462,  437,  452,  448,  461,  440,  434,
  429,  428,  478,  422,  421,  420,  418,  415,  397,  412,
  381,  417,  411,  256,  403,  406,  399,  398,  396,  395,
  394,  393,  387,  388,  389,  371,  377,  380,  375,  373,
  370,  362,  369,  368,  365,  359,  355,  351,  350,  348,
  342,  345,  255,  339,  344,  390,  337,  335,  334,  333,

  328,  240,  321,  318,  331,  330,  326,  316,  311,  317,
  310,  302,  312,  300,  299,  298,  297,  295,  291,  242,
  293,  292,  198,  284,  282,  287,  241,  230,  281,  285,
  201,  235,  196,  224,  195,  225,  223,  222,  220,  216,
  194,  215,  214,  192,  199,  185,  177,  186,  172,  180,
  173,  114,  170,  183,  169,  166,  165,  136,  145,  168,
  160,  143,  128,  162,  153,   88,    0,    7,  865,  985,
  579,  588,   12,    3,  556,  579,    0,    0,    5,   86,
 1015,  206,  864, 1105,  589,  596,    2,  583,  557,  586,
    0,    0,  637, 1013,    0, 1085, 1063,  575,  862,  577,

  598,    9,  558,  574,    0,  987,    0,  860, 1039, 1006,
    0,    4,  999,  671,    0,   32,  151,  908,  605,  115,
  130,  117,  112,  111,  105,   81,  110,   77,  104,  109,
   31,   90,  100,   27,   60,   94,   74,   10,   71,   59,
   49,   52,   55,   51,   26,   46,   50,   38,    4,    3,
   18,   35,   12,    2,    8,    1,    9,  262
};

static short accept [359] = {
    0,

    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,
   11,   12,   13,   14,   15,   16,   17,   18,   19,   20,
   21,   22,   23,   24,   25,   26,   27,   28,   29,   30,
   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
   41,   42,   43,   44,   45,   46,   47,   48,   49,   50,
   51,   52,   53,   54,   55,   56,   57,   58,   59,   60,
   61,   62,   63,   64,   65,   66,   67,   68,   69,   70,
   71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
   81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
   91,   92,   93,   94,   95,   96,   97,   98,   99,  100,

  101,  102,  103,  104,  105,  106,  107,    0,  107,    0,
  107,    0,    0,    0,   48,    0,    0,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,

   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   58,    0,    0,    0,
   49,   49,   49,   49,   49,   49,   49,   57,    0,    0,
    0,    0,    0,   51,   51,   51,   51,   51,   51,   51,
   51,   56,    0,    0,   55,   55,    0,   50,    0,   50,

   50,   50,   50,   50,   50,    0,   54,    0,   54,    0,
   53,    0,    0,    0,  107,    0,    0,    0,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48
};

static short def [359] = {
  358,

    2,   52,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  282,  297,
  284,   60,  313,  309,  306,  294,  281,  270,    1,   61,
   62,   63,   64,   65,   66,   67,   68,   69,   70,   73,
    1,   87,   74,   75,   76,   77,   78,   79,   80,   81,
   82,   83,   84,   85,   88,    1,  103,   89,   90,   91,
   95,  281,    1,    1,  105,    1,    1,    1,    1,    1,

    1,    1,    1,    1,  267,    1,    0,    1,  110,   -1,
  314,   50,  280,  117,  358,  270,    1,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,

  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  277,  280,  281,    1,
  276,    1,  276,    1,    1,  304,  278,  291,  280,   50,
  294,  317,  294,  297,  272,  290,    1,  290,    1,    1,
  292,  295,  306,  310,  305,   54,  281,  304,  310,    1,

  304,    1,    1,    1,  307,  270,  311,  313,  310,    1,
  315,  314,  306,   -1,   -1,  112,  113,    0,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,   -1
};

static unsigned char meta [79] = {
    0,

    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0
};

static unsigned char class [256] = {
   19,

   20,   19,   21,   19,   19,   19,   19,   19,   15,   11,
   15,   15,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   15,   22,    9,   19,   19,   23,   24,   17,   25,
   26,   27,   28,   29,   10,   30,   31,    7,    3,    3,
    3,    3,    3,    3,    3,   32,    8,   33,   34,   35,
   36,   37,   18,   19,   38,   39,   40,   14,   12,   41,
   42,   16,   43,   16,   16,   44,   16,   45,   16,   13,
   16,   16,   46,   47,   48,   16,   16,    2,   16,   16,
   49,    4,   50,   51,   52,   19,   53,   54,   55,   56,

   57,    5,   58,   59,   60,   16,   61,   62,   63,   64,
   65,   66,   16,   67,   68,   69,   70,    6,   71,   72,
   73,   74,   75,   76,   77,   78,   19,   19,   19,   19,
   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,

   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   19,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   19,   19,   19,   19
};/*
.. table based tokenizer. Assumes,
.. (a) state '0' is the starting dfa.
1
.. (c) maximum depth of 1 for "def" (fallback) chaining.
.. (d) Doesn't use meta class.
.. (e) Token value '0' : rejected. No substring matched
.. (f) accept value in [1, ntokens] for accepted tokens
*/

static char lxrholdchar = '\0';
#define LXR_MAXDEPTH    1

int lxr_lex () {

  int isEOF = 0;

  do {

    *lxrstrt = lxrholdchar;       /* put back the holding character */

    int c, ec, s = 0,                             /* state iterator */
      acc_token = 0,                         /* last accepted token */
      acc_len = 0,             /* length of the last accepted state */
      len = 0,                   /* consumed length for the current */
      depth;                                   /* depth of fallback */

    /*
    .. fixme : if(BOL), do the transition here. ec [BOL] = 0;
    .. fixme : copy the content of lxr_input (), rather than calling
    .. lxr_input ();
    */
    while ( s != -1 )  {

      c = lxr_input ();
      if (c == EOF) { if (!len) isEOF = 1; break; }
      ec = class [c];
    
      /*
      .. fixme : if(EOL/EOF), do the transition here. ec [EOL] = 1;
      */

      ++len;

      /* dfa transition by c */
      depth = 0;
      while ( s != -1 && check [ base [s] + ec ] != s ) {
        /* note : no meta class as of now */
        /*
        .. assumes next[c] is empty if number of fallbacks reaches
        .. LXR_MAXDEPTH
        */
        s = depth++ == LXR_MAXDEPTH ? -1 : def [s];
      }
      if ( s != -1 )
        s = next [base[s] + ec];

      if ( s != -1 && accept [s] ) {
        acc_len = len;
        acc_token = accept [s];
      }
    }

    /*
    .. Update with new holding character & it's location.
    */
    char * lxrholdloc = lxrstrt + (acc_len ? acc_len : 1);
    lxrholdchar = *lxrholdloc;
    *lxrholdloc = '\0';

    /*
    .. handling accept/reject. In case of accepting, corresponding
    .. action snippet will be executed. In case of reject (unknown,
    .. pattern) lexer will simply consume without any warning. If you
    .. don't want it to go unnoticed in suchcases, you can add an "all
    .. catch" regex (.) in the last line of the rule section after all
    .. the expected patterns are defined. A sample lex snippet is
    ,, given below
    ..
    .. [_a-zA-Z][_a-zA-Z0-9]*                   { return ( IDENT );  }
    .. .                                        { return ( ERROR );  }
    */

    do {   /* Just used a newer block to avoid clash of identifiers */
      switch ( acc_token ) {

      case 1 :
        printf ("\ntoken [  1] %s", lxrstrt);
        break;
      case 2 :
        printf ("\ntoken [  2] %s", lxrstrt);
        break;
      case 3 :
        printf ("\ntoken [  3] %s", lxrstrt);
        break;
      case 4 :
        printf ("\ntoken [  4] %s", lxrstrt);
        break;
      case 5 :
        printf ("\ntoken [  5] %s", lxrstrt);
        break;
      case 6 :
        printf ("\ntoken [  6] %s", lxrstrt);
        break;
      case 7 :
        printf ("\ntoken [  7] %s", lxrstrt);
        break;
      case 8 :
        printf ("\ntoken [  8] %s", lxrstrt);
        break;
      case 9 :
        printf ("\ntoken [  9] %s", lxrstrt);
        break;
      case 10 :
        printf ("\ntoken [ 10] %s", lxrstrt);
        break;
      case 11 :
        printf ("\ntoken [ 11] %s", lxrstrt);
        break;
      case 12 :
        printf ("\ntoken [ 12] %s", lxrstrt);
        break;
      case 13 :
        printf ("\ntoken [ 13] %s", lxrstrt);
        break;
      case 14 :
        printf ("\ntoken [ 14] %s", lxrstrt);
        break;
      case 15 :
        printf ("\ntoken [ 15] %s", lxrstrt);
        break;
      case 16 :
        printf ("\ntoken [ 16] %s", lxrstrt);
        break;
      case 17 :
        printf ("\ntoken [ 17] %s", lxrstrt);
        break;
      case 18 :
        printf ("\ntoken [ 18] %s", lxrstrt);
        break;
      case 19 :
        printf ("\ntoken [ 19] %s", lxrstrt);
        break;
      case 20 :
        printf ("\ntoken [ 20] %s", lxrstrt);
        break;
      case 21 :
        printf ("\ntoken [ 21] %s", lxrstrt);
        break;
      case 22 :
        printf ("\ntoken [ 22] %s", lxrstrt);
        break;
      case 23 :
        printf ("\ntoken [ 23] %s", lxrstrt);
        break;
      case 24 :
        printf ("\ntoken [ 24] %s", lxrstrt);
        break;
      case 25 :
        printf ("\ntoken [ 25] %s", lxrstrt);
        break;
      case 26 :
        printf ("\ntoken [ 26] %s", lxrstrt);
        break;
      case 27 :
        printf ("\ntoken [ 27] %s", lxrstrt);
        break;
      case 28 :
        printf ("\ntoken [ 28] %s", lxrstrt);
        break;
      case 29 :
        printf ("\ntoken [ 29] %s", lxrstrt);
        break;
      case 30 :
        printf ("\ntoken [ 30] %s", lxrstrt);
        break;
      case 31 :
        printf ("\ntoken [ 31] %s", lxrstrt);
        break;
      case 32 :
        printf ("\ntoken [ 32] %s", lxrstrt);
        break;
      case 33 :
        printf ("\ntoken [ 33] %s", lxrstrt);
        break;
      case 34 :
        printf ("\ntoken [ 34] %s", lxrstrt);
        break;
      case 35 :
        printf ("\ntoken [ 35] %s", lxrstrt);
        break;
      case 36 :
        printf ("\ntoken [ 36] %s", lxrstrt);
        break;
      case 37 :
        printf ("\ntoken [ 37] %s", lxrstrt);
        break;
      case 38 :
        printf ("\ntoken [ 38] %s", lxrstrt);
        break;
      case 39 :
        printf ("\ntoken [ 39] %s", lxrstrt);
        break;
      case 40 :
        printf ("\ntoken [ 40] %s", lxrstrt);
        break;
      case 41 :
        printf ("\ntoken [ 41] %s", lxrstrt);
        break;
      case 42 :
        printf ("\ntoken [ 42] %s", lxrstrt);
        break;
      case 43 :
        printf ("\ntoken [ 43] %s", lxrstrt);
        break;
      case 44 :
        printf ("\ntoken [ 44] %s", lxrstrt);
        break;
      case 45 :
        printf ("\ntoken [ 45] %s", lxrstrt);
        break;
      case 46 :
        printf ("\ntoken [ 46] %s", lxrstrt);
        break;
      case 47 :
        printf ("\ntoken [ 47] %s", lxrstrt);
        break;
      case 48 :
        printf ("\ntoken [ 48] %s", lxrstrt);
        break;
      case 49 :
        printf ("\ntoken [ 49] %s", lxrstrt);
        break;
      case 50 :
        printf ("\ntoken [ 50] %s", lxrstrt);
        break;
      case 51 :
        printf ("\ntoken [ 51] %s", lxrstrt);
        break;
      case 52 :
        printf ("\ntoken [ 52] %s", lxrstrt);
        break;
      case 53 :
        printf ("\ntoken [ 53] %s", lxrstrt);
        break;
      case 54 :
        printf ("\ntoken [ 54] %s", lxrstrt);
        break;
      case 55 :
        printf ("\ntoken [ 55] %s", lxrstrt);
        break;
      case 56 :
        printf ("\ntoken [ 56] %s", lxrstrt);
        break;
      case 57 :
        printf ("\ntoken [ 57] %s", lxrstrt);
        break;
      case 58 :
        printf ("\ntoken [ 58] %s", lxrstrt);
        break;
      case 59 :
        printf ("\ntoken [ 59] %s", lxrstrt);
        break;
      case 60 :
        printf ("\ntoken [ 60] %s", lxrstrt);
        break;
      case 61 :
        printf ("\ntoken [ 61] %s", lxrstrt);
        break;
      case 62 :
        printf ("\ntoken [ 62] %s", lxrstrt);
        break;
      case 63 :
        printf ("\ntoken [ 63] %s", lxrstrt);
        break;
      case 64 :
        printf ("\ntoken [ 64] %s", lxrstrt);
        break;
      case 65 :
        printf ("\ntoken [ 65] %s", lxrstrt);
        break;
      case 66 :
        printf ("\ntoken [ 66] %s", lxrstrt);
        break;
      case 67 :
        printf ("\ntoken [ 67] %s", lxrstrt);
        break;
      case 68 :
        printf ("\ntoken [ 68] %s", lxrstrt);
        break;
      case 69 :
        printf ("\ntoken [ 69] %s", lxrstrt);
        break;
      case 70 :
        printf ("\ntoken [ 70] %s", lxrstrt);
        break;
      case 71 :
        printf ("\ntoken [ 71] %s", lxrstrt);
        break;
      case 72 :
        printf ("\ntoken [ 72] %s", lxrstrt);
        break;
      case 73 :
        printf ("\ntoken [ 73] %s", lxrstrt);
        break;
      case 74 :
        printf ("\ntoken [ 74] %s", lxrstrt);
        break;
      case 75 :
        printf ("\ntoken [ 75] %s", lxrstrt);
        break;
      case 76 :
        printf ("\ntoken [ 76] %s", lxrstrt);
        break;
      case 77 :
        printf ("\ntoken [ 77] %s", lxrstrt);
        break;
      case 78 :
        printf ("\ntoken [ 78] %s", lxrstrt);
        break;
      case 79 :
        printf ("\ntoken [ 79] %s", lxrstrt);
        break;
      case 80 :
        printf ("\ntoken [ 80] %s", lxrstrt);
        break;
      case 81 :
        printf ("\ntoken [ 81] %s", lxrstrt);
        break;
      case 82 :
        printf ("\ntoken [ 82] %s", lxrstrt);
        break;
      case 83 :
        printf ("\ntoken [ 83] %s", lxrstrt);
        break;
      case 84 :
        printf ("\ntoken [ 84] %s", lxrstrt);
        break;
      case 85 :
        printf ("\ntoken [ 85] %s", lxrstrt);
        break;
      case 86 :
        printf ("\ntoken [ 86] %s", lxrstrt);
        break;
      case 87 :
        printf ("\ntoken [ 87] %s", lxrstrt);
        break;
      case 88 :
        printf ("\ntoken [ 88] %s", lxrstrt);
        break;
      case 89 :
        printf ("\ntoken [ 89] %s", lxrstrt);
        break;
      case 90 :
        printf ("\ntoken [ 90] %s", lxrstrt);
        break;
      case 91 :
        printf ("\ntoken [ 91] %s", lxrstrt);
        break;
      case 92 :
        printf ("\ntoken [ 92] %s", lxrstrt);
        break;
      case 93 :
        printf ("\ntoken [ 93] %s", lxrstrt);
        break;
      case 94 :
        printf ("\ntoken [ 94] %s", lxrstrt);
        break;
      case 95 :
        printf ("\ntoken [ 95] %s", lxrstrt);
        break;
      case 96 :
        printf ("\ntoken [ 96] %s", lxrstrt);
        break;
      case 97 :
        printf ("\ntoken [ 97] %s", lxrstrt);
        break;
      case 98 :
        printf ("\ntoken [ 98] %s", lxrstrt);
        break;
      case 99 :
        printf ("\ntoken [ 99] %s", lxrstrt);
        break;
      case 100 :
        printf ("\ntoken [100] %s", lxrstrt);
        break;
      case 101 :
        printf ("\ntoken [101] %s", lxrstrt);
        break;
      case 102 :
        printf ("\ntoken [102] %s", lxrstrt);
        break;
      case 103 :
        printf ("\ntoken [103] %s", lxrstrt);
        break;
      case 104 :
        printf ("\ntoken [104] %s", lxrstrt);
        break;
      case 105 :
        printf ("\ntoken [105] %s", lxrstrt);
        break;
      case 106 :
        printf ("\ntoken [106] %s", lxrstrt);
        break;
      case 107 :
        printf ("\ntoken [107] %s", lxrstrt);
        break;

        default :                                /* Unknown pattern */
          /*% if (isEOF) replace this line with any snippet reqd   %*/
      }
    } while (0);

    /* Place reading idx just after the last character of the token */
    lxrbptr = lxrstrt = lxrholdloc;

  } while (!isEOF);    /* Will stop when the first character is EOF */

  return EOF;
}

int main () {
  lxr_lex ();
  return 0;
}