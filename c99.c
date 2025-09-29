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
      return ( (unsigned char) (lxrprevchar = *lxrbptr++) );
      
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


static short lxr_check [1110] = {
    1,

    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,  289,  274,   99,
  104,   95,   72,   87,   88,  303,  109,  302,  275,  117,
  287,   73,  105,  300,  114,  290,  103,  103,  272,  276,

  285,  101,  101,   94,  107,   98,  304,  288,  107,  301,
   97,  300,  273,   98,  271,   94,  272,  290,  285,  286,
  100,  276,  298,  356,  100,  308,  313,  354,  304,  100,
  313,  313,  105,  349,  310,  268,   97,   97,  310,  310,
  294,  102,  281,  308,  294,  294,  281,  281,  299,  283,
  338,  102,  270,  102,  102,  313,  270,  270,  269,  116,
  306,  293,   57,  310,  306,  306,  299,  283,   93,  294,
  334,  281,   93,   93,  279,  350,  269,  116,   60,  293,
   60,  270,  297,  350,   60,   58,  297,  297,  268,  306,
   59,  297,  309,  357,   54,   93,   56,   93,   57,  284,

  353,   57,  351,  284,  355,  357,  337,  337,  352,  297,
  348,  297,  353,   60,  351,  355,  296,   60,  352,   57,
  348,   58,  347,  296,   58,  344,   59,  279,  309,   59,
   54,  309,   56,   54,  347,   56,  297,  344,  346,   60,
  284,   55,   58,  331,  284,   55,   55,   59,  346,  309,
   55,   54,  296,   56,  266,  296,   51,  345,  284,   52,
   51,   51,  256,  342,  345,   52,  284,  341,  296,   52,
   55,  256,  343,  296,  343,  342,  316,  316,  316,  316,
  316,  340,  316,  336,  336,   51,  339,   52,  340,   52,
  316,  316,  330,  339,  335,   55,  328,   51,  328,  335,

  252,   51,  112,  112,  112,  112,  112,  333,  112,  329,
  333,  247,  329,  332,   52,   51,  112,  112,  332,  341,
  326,  327,  326,   51,  327,  325,  316,  316,  323,   52,
  265,  324,  244,  322,  320,  323,  265,  316,  325,  320,
  316,  324,  316,  322,  330,  316,  280,  321,  280,  241,
  280,  280,  112,  112,  259,  280,  113,  280,  113,  321,
  113,  113,  254,  112,  259,  113,  112,  113,  112,  264,
   50,  112,  261,  264,  260,  280,  263,  263,  260,  261,
  258,  280,  280,  280,  280,  113,  262,   50,  258,  262,
  255,  113,  113,  113,  113,  255,  280,  280,  280,  280,

  280,   50,  235,  233,  235,   50,  113,  113,  113,  113,
  113,  317,  257,  317,  254,  317,  317,  253,  257,   50,
  317,  251,  317,   50,  251,  253,  250,   50,  248,  249,
  246,  240,  248,  245,  231,  231,  223,  250,  246,  243,
  317,  249,  242,  245,  240,  239,  317,  317,  317,  317,
  238,  243,  230,  239,  242,  221,  237,  230,  221,  236,
  238,  317,  317,  317,  317,  317,  282,  236,  282,  237,
  282,  282,  234,  202,  232,  282,  229,  282,  232,  228,
  226,  229,  234,  227,  226,  227,  225,  228,  224,  219,
  193,  219,  222,  282,  225,  282,  222,  220,  224,  218,

  164,  282,  282,  282,  282,  217,  215,  218,  216,  220,
  210,  216,  217,  215,  210,  161,  282,  282,  282,  282,
  282,  358,  358,  214,  358,  358,  358,  358,  214,  190,
  214,  358,  358,  358,  212,  358,  213,  211,  214,  190,
  213,  212,  213,  209,  208,  204,  201,  208,  213,  211,
  207,  358,  207,  159,  201,  209,  204,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  196,  166,
  166,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  206,  203,  205,  200,  199,  206,

  205,  198,  197,  195,  194,  200,  203,  195,  188,  198,
  191,  199,  192,  192,  197,  189,  182,  194,  189,  186,
  196,  188,  191,  192,  192,  187,  182,  186,  185,  184,
  187,  184,  185,  187,  183,  181,  178,  180,  179,  183,
  178,  179,  177,  176,  177,  153,  170,  181,  180,  180,
  175,  174,  173,  175,  174,  176,  172,  173,  171,  170,
  169,  168,  167,  172,  168,  167,  165,  142,  169,  171,
  160,  163,  168,  165,  163,  162,  163,  160,  158,  162,
  156,  157,  155,  152,  151,  154,  158,  157,  154,  162,
  156,  151,  141,  150,  155,  150,  152,  153,  149,  141,

  148,  148,  148,  149,  148,  148,  147,  148,  148,  148,
  146,  147,  140,  146,  148,  145,  145,  139,  147,  148,
  144,  138,  144,  144,  137,  137,  139,  136,  140,  135,
  133,  132,  144,  132,  144,  133,  131,  131,  130,  129,
  129,  128,  127,  128,  126,  125,  127,  124,  123,  126,
  122,  125,  121,  120,  124,  119,  123,  118,  120,   48,
  122,   47,  115,  121,   46,  120,  115,   45,   44,   43,
  119,   42,   41,  118,  115,   40,   39,   38,   37,   36,
   35,  135,   34,   33,   32,   31,   30,   29,  135,   28,
   27,   26,   25,   24,   23,   22,   21,   20,   19,   18,

   17,   16,   15,   14,   13,   12,   11,   10,    9,    8,
    7,    6,    5,    4,  143,  143,  319,  134,   49,   11,
  312,   -1,   -1,   -1,   -1,  134,   49,   -1,   -1,   -1,
   -1,  143,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  134,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  319,  134,   49,   -1,   -1,   -1,
   -1,   -1,  134,  314,  314,  314,  314,  314,  314,  314,
  314,  314,   -1,  314,  314,  314,  314,  314,   -1,  314,
  314,  314,  314,  314,  314,  314,  314,  314,  314,  314,

  314,  314,  314,  314,  314,  314,  314,  314,  314,  314,
  314,  314,  314,  314,  314,  314,  314,  314,  314,  314,
  314,  314,  314,  314,  314,  314,  314,  314,  314,  314,
  314,  314,  314,  314,  314,  314,  314,  314,  314,  314,
  314,  314,  314,  314,  314,  314,  314,  314,  314,  314,
  314,  110,  110,  110,  110,  110,  110,  110,  110,  110,
   -1,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,

  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  318,
  318,  318,  318,  318,  318,  318,  318,  318,   -1,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,

  318,  318,  318,  318,  318,  318,  318,  318,   -1
};

static short lxr_next [1110] = {
  266,

   51,  315,  187,  169,   52,   51,  108,   97,  107,  266,
  266,  266,  107,  266,  111,  106,  315,  315,  315,   95,
  101,   94,   89,   90,   99,   98,   86,   93,  100,   51,
   87,   83,  102,   88,  103,  266,  266,  266,  266,  266,
  266,   49,  266,  266,  266,   49,   91,   92,  104,  148,
  199,  224,  213,  123,  214,  200,  266,  135,  266,  218,
  266,  266,  266,  266,  193,  144,  140,  134,  177,  266,
  266,  266,   84,  105,   85,   96,  315,  291,  277,   66,
   70,   82,   62,   92,   81,  305,   61,  305,  277,  110,
  291,   63,   71,  302,  117,  291,   80,   72,  274,  277,

  287,   68,   85,   77,  107,   74,  305,  290,  107,  304,
   75,  303,  276,   64,  276,   69,  275,  291,  289,  290,
    2,  277,  304,    9,  318,  313,   54,   12,  305,   67,
   54,   54,   78,   13,  309,  116,   65,   76,  309,  309,
   57,   84,   58,  313,   57,   57,   58,   58,  310,  294,
  337,   91,   59,   73,   79,   54,   59,   59,  281,  270,
   56,  306,  292,  309,   56,   56,  310,  294,   55,   57,
   20,   58,   55,   55,  269,  266,  281,  270,  110,  306,
   60,   59,  297,    4,   60,  278,  297,  297,  116,   56,
  267,  308,  307,  266,  311,  109,  295,   55,  292,  284,

  266,  292,  266,  284,  266,    8,   27,  266,  266,  296,
  266,  297,   10,  117,  350,  358,  295,  117,    7,  292,
   17,  278,  266,  293,  278,  266,  267,  269,  307,  267,
  311,  307,  295,  311,   16,  295,  308,   24,  266,  114,
  286,   55,  278,   36,  285,   55,   55,  267,  345,  307,
  299,  311,  295,  295,  266,  295,   51,  266,  288,  282,
   51,   51,  266,  266,   15,  297,  285,   28,  293,  308,
   55,  255,  344,  295,  266,   26,  312,  312,  312,  312,
  312,  266,  312,   34,  266,   51,  266,  296,   25,  297,
  312,  312,   32,  338,  266,  299,   29,  301,  266,  354,

  119,  300,  110,  110,  110,  110,  110,  266,  110,  266,
  332,  122,  328,  266,  308,  298,  110,  110,  331,  266,
   40,  266,  266,  300,  326,  266,  312,  312,  266,  282,
  266,  266,  243,  266,  266,   33,  264,  312,  324,   41,
  312,  340,  312,  342,  266,  317,  279,  266,  279,  346,
  279,  279,  110,  110,  266,  279,  110,  279,  110,   30,
  110,  110,   39,  110,  118,  110,  110,  110,  110,  263,
  283,  113,  266,  266,  259,  279,  266,  262,  266,  260,
  266,  279,  279,  279,  279,  110,  266,  268,  257,  261,
  266,  110,  110,  110,  110,  252,  279,  279,  279,  279,

  279,  273,  266,  232,  234,  272,  110,  110,  110,  110,
  110,  312,  266,  312,  266,  312,  312,  266,  256,  271,
  312,  266,  312,  283,  250,  341,  266,  272,  352,  266,
  266,  266,  266,  266,  230,  266,  222,  249,  245,  266,
  312,  247,  266,  244,  238,  266,  312,  312,  312,  312,
  266,  240,  229,  323,  241,  266,  266,  266,  220,  266,
  124,  312,  312,  312,  312,  312,   50,  225,   50,  125,
   50,   50,  266,  182,  231,   50,  266,   50,  266,  266,
  126,  228,  233,  226,  266,  266,  266,  227,  266,  128,
  192,  266,  127,  280,  320,   50,  266,  266,  223,  266,

  336,   50,   50,   50,   50,  266,  266,  217,  266,  219,
  209,  215,  129,  170,  266,  160,   50,   50,   50,   50,
   50,  266,  266,  266,  266,  266,  266,  266,  171,  266,
  237,  266,  266,  266,  266,  266,  355,  266,  242,  189,
  266,  180,  248,  266,  266,  266,  266,  207,  212,  210,
  206,  266,  266,  156,  322,  208,  203,  266,  266,  266,
  266,  266,  266,  266,  266,  266,  266,  266,  195,  266,
  165,  266,  266,  266,  266,  266,    6,  266,  266,  266,
  266,  266,  266,  266,  266,  266,  266,  266,  266,  266,
  266,  266,  266,  266,  205,  266,  204,  266,  266,  266,

  266,  266,  266,  194,  266,  197,  202,  266,  266,   18,
  266,  351,  266,  251,  198,  266,  266,  183,  343,  266,
  266,  184,  190,  191,  325,  266,  181,  185,  347,  321,
  186,  266,  266,  348,  266,  266,  176,  266,  266,  353,
  266,  172,  266,  266,  174,  152,  266,   46,  357,  216,
  335,  266,  266,  266,  173,  333,  266,  130,  266,  356,
  266,  266,  266,  334,  239,  166,  266,  253,  163,  349,
  266,  266,  167,  164,  131,  155,  178,  159,  266,  266,
  266,  266,  266,  266,  266,  266,  157,  327,  132,  188,
  154,  150,  266,  149,  329,  266,  151,  266,  133,  115,

  147,  236,  258,  266,  161,  265,  266,  246,  211,  235,
  266,  146,  266,  145,  153,  266,  141,  266,  158,  266,
  266,  137,  201,  143,  136,  266,  138,  330,  139,   19,
   48,   43,  162,  266,  221,  266,   35,  266,   37,  266,
   22,  266,  266,   31,  266,  266,    5,  266,  196,   47,
  266,   14,  266,  266,   45,  266,   11,  266,  179,  266,
   23,  266,  121,   38,  266,   21,  266,  266,  266,  266,
   42,  266,  266,   44,  254,  266,  266,  266,  266,  266,
  266,  266,  266,  266,  266,  266,  266,  266,  120,  266,
  266,  266,  266,  266,  266,  266,  266,  266,  266,  266,

  266,  266,  266,  266,  266,  266,  266,  266,  266,  266,
  266,  266,  266,  266,  266,  339,  110,  110,  110,  175,
   53,    0,    0,    0,    0,  314,  314,    0,    0,    0,
    0,  142,    0,    0,    0,    0,    0,    0,    0,    0,
  319,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  266,  266,  266,    0,    0,    0,
    0,    0,  168,  312,  312,  316,  312,  312,  312,  312,
  312,  312,    0,  312,  312,  312,  312,  312,    0,  312,
  312,  312,  312,  312,  312,  312,  312,  312,  312,  312,

  312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
  312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
  312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
  312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
  312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
  312,  110,  110,  112,  110,  110,  110,  110,   60,  110,
    0,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,

  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  110,
  110,  110,  110,  110,  110,  110,  110,  110,  110,  318,
  318,  318,  318,  318,  318,  318,  318,  318,    0,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
  318,  318,  318,  318,  318,  318,  318,  318,  318,  318,

  318,  318,  318,  318,  318,  318,  318,    3,    0
};

static short lxr_base [359] = {
    0,

    0,    0,    0,  759,  758,  757,  756,  755,  754,  753,
  752,  751,  750,  749,  748,  747,  746,  745,  744,  743,
  742,  741,  740,  739,  738,  737,  736,  735,  733,  732,
  731,  730,  729,  728,  726,  725,  724,  723,  722,  721,
  718,  717,  715,  714,  713,  710,  707,  705,  812,  360,
  256,  260,    0,  192,  241,  194,  160,  183,  188,  172,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   49,   58,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   49,   51,    0,    0,
    0,    0,  168,   82,   48,    0,  103,   80,   46,   96,

   68,  121,   63,   47,   59,    0,   96,    0,   59,  952,
    0,  302,  356,   65,  712,  152,   83,  703,  701,  699,
  698,  696,  694,  693,  691,  690,  688,  687,  685,  684,
  683,  679,  681,  811,  727,  673,  671,  667,  663,  658,
  638,  613,  760,  666,  661,  656,  652,  665,  649,  641,
  630,  629,  643,  631,  628,  626,  627,  624,  499,  616,
  461,  625,  617,  446,  612,  515,  608,  607,  606,  592,
  604,  602,  598,  597,  599,  589,  588,  586,  584,  583,
  581,  562,  580,  577,  578,  565,  571,  554,  561,  475,
  556,  558,  436,  550,  553,  566,  548,  547,  544,  543,

  492,  419,  541,  491,  546,  545,  498,  490,  489,  460,
  483,  480,  486,  469,  452,  454,  451,  445,  437,  443,
  401,  442,  382,  434,  432,  430,  431,  425,  422,  403,
  381,  424,  349,  418,  348,  405,  402,  396,  391,  377,
  295,  388,  385,  278,  379,  376,  257,  378,  375,  372,
  367,  246,  363,  360,  336,  208,  358,  326,  300,  324,
  318,  332,  322,  319,  276,  200,    0,  125,  151,  152,
   55,   57,   71,   37,   29,   54,    0,    0,  164,  346,
  142,  466,  142,  199,   59,   78,   49,   48,   18,   50,
    0,    0,  154,  140,    0,  214,  182,   63,  141,   52,

   68,   46,   26,   61,    0,  160,    0,  118,  190,  134,
    0,  806,  126,  874,    0,  276,  411, 1030,  810,  280,
  293,  279,  274,  277,  271,  268,  267,  244,  255,  290,
  189,  259,  253,  116,  240,  230,  153,   96,  232,  227,
  265,  209,  220,  171,  203,  184,  168,  156,   79,  121,
  148,  154,  146,   73,  150,   69,  139,  522
};

static short lxr_accept [359] = {
    0,

    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,
   10,   11,   12,   13,   14,   15,   16,   17,   18,   19,
   20,   21,   22,   23,   24,   25,   26,   27,   28,   29,
   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,
   40,   41,   42,   43,   44,   45,   46,   47,   48,   49,
   50,   51,   52,   53,   54,   55,   56,   57,   58,   59,
   60,   61,   62,   63,   64,   65,   66,   67,   68,   69,
   70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
   80,   81,   82,   83,   84,   85,   86,   87,   88,   89,
   90,   91,   92,   93,   94,   95,   96,   97,   98,   99,

  100,  101,  102,  103,  104,  105,  106,  107,    0,    0,
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

static short lxr_def [359] = {
    1,

   -1,    3,   53,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  282,
  297,  284,   61,  313,  309,  306,  294,  281,  270,    2,
   62,   63,   64,   65,   66,   67,   68,   69,   70,   71,
   74,   95,    2,   75,   76,   77,   78,   79,   80,   81,
   82,   83,   84,   85,   86,   89,    2,    2,   90,   91,
   92,   96,  306,    2,  104,  106,    2,    2,    2,    2,

    2,    2,    2,   99,   73,  267,    2,  110,    2,   -1,
  314,  316,  280,  117,  358,  270,    2,  358,  358,  358,
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
  358,  358,  358,  358,  358,  358,  277,  280,  281,    2,
  276,    2,  276,    2,    2,    2,  278,  291,  280,    2,
  294,  317,  294,  297,  272,  290,    2,  290,    2,    2,
  292,  295,  306,  310,  305,   55,  306,  304,  310,    2,

  304,    2,    2,    2,  307,  270,  311,  313,  310,  313,
  315,  314,    2,   -1,   -1,    2,  113,  108,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,  358,  358,  358,
  358,  358,  358,  358,  358,  358,  358,   -1
};

static unsigned char lxr_meta [80] = {
    0,

    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0
};

static unsigned char lxr_class [256] = {
   17,

   18,   17,   19,   17,   17,   17,   17,   17,   13,    9,
   13,   13,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   13,   20,    7,   17,   17,   21,   22,   15,   23,
   24,   25,   26,   27,    8,   28,   29,    5,    1,    1,
    1,    1,    1,    1,    1,   30,    6,   31,   32,   33,
   34,   35,   16,   17,   36,   37,   38,   12,   10,   39,
   40,   14,   41,   14,   14,   42,   14,   43,   14,   11,
   14,   14,   44,   45,   46,   14,   14,    0,   14,   14,
   47,    2,   48,   49,   50,   17,   51,   52,   53,   54,

   55,    3,   56,   57,   58,   14,   59,   60,   61,   62,
   63,   64,   14,   65,   66,   67,   68,    4,   69,   70,
   71,   72,   73,   74,   75,   76,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,

   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17,   17,   17,   17,   17,   17,
   17,   17,   17,   17,   17
};
#define lxrEOLclass  78

/*
.. table based tokenizer. Assumes,
.. (a) state '0' is the starting dfa.
.. (b) There is no zero-length tokens,
.. (c) maximum depth of 1 for "def" (fallback) chaining.
.. (d) Doesn't use meta class.
.. (e) Token value '0' : rejected. No substring matched
.. (f) accept value in [1, ntokens] for accepted tokens
*/

static char lxrholdchar = '\0';
static int  lxrBOLstatus = 1;
#define LXR_MAXDEPTH    1
#define LXRDEAD        -1

int lxr_lex () {
  int isEOF = 0;

  do {                  /* Loop looking the longest token until EOF */

    *lxrstrt = lxrholdchar; /* put back the holding character       */

    int uchar, class,       /* input character & it's class         */
      state = lxrBOLstatus, /* start with 0/1 depending on BOL flag */
      last_state = LXRDEAD, /* if EOL transition failed, go back    */
      acc_token = 0,        /* last accepted token                  */
      acc_len = 0,          /* length of the last accepted state    */
      len = 0,              /* consumed length for the current      */
      depth,                /* depth of fallback                    */
      lxrEOLstatus = 
        ( lxrbptr == lxrEOF ) || (*lxrbptr == '\n');

    do {                            /* Transition loop until reject */

      /*
      .. Before running the transition corresponding to byte input
      .. from the file, we do the EOL transition if the next byte is
      .. '\n' or has reached EOF
      */
      if (lxrEOLstatus) {
        class = lxrEOLclass; lxrEOLstatus = 0;
        last_state = state;
      }
      else {
        /*
        .. Consume the byte from file/buffer and update EOL status
        */
        if ( lxrbptr[1] == '\0' && lxrEOF == NULL )
          lxr_buffer_update ();
        if (lxrbptr == lxrEOF) {
          if (len == 0) isEOF = 1;
          break;
        }
        uchar = (unsigned char) *lxrbptr++;
        len++;
        class = lxr_class [uchar];
        lxrEOLstatus = ( lxrbptr == lxrEOF ) || (*lxrbptr == '\n');
      }

      /*
      .. find the transition corresponding to the class using check/
      .. next tables and if not found in [base, base + nclass), use
      .. the fallback
      */
      depth = 0;
      while ( state != LXRDEAD && 
        ((int) lxr_check [lxr_base [state] + class] != state) ) {
        /*
        .. Note: (a) No meta class as of now. (b) assumes transition
        .. is DEAD if number of fallbacks reaches LXR_MAXDEPTH
        */
        state = (depth++ == LXR_MAXDEPTH) ? LXRDEAD : 
          (int) lxr_def [state];
      }

      /*
      .. Update the most eligible token using the criteria
      .. (a) Longest token (b) In case of clash use the first token
      .. defined in the lexer file
      */
      if ( state != LXRDEAD ) {
        state = (int) lxr_next [lxr_base[state] + class];
        if ( state != LXRDEAD && lxr_accept [state] &&
          ( len > acc_len || (int) lxr_accept [state] < acc_token) )
        {
          acc_len = len;
          acc_token = (int) lxr_accept [state];
        }
      }

      /*
      .. We undo the EOL transition and proceed further looking for
      .. longer tokens.
      */
      if (class == lxrEOLclass)
        state = last_state;

    } while ( state != LXRDEAD ); 

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
    lxrBOLstatus = (lxrholdloc [-1] == '\n');

  } while (!isEOF);    /* Will stop when the first character is EOF */

  return EOF;
}

#undef LXRDEAD

int main () {
  lxr_lex ();
  return 0;
}