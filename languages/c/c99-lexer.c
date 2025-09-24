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

static char lxrdummy[3] = {0}; //'\0', 'x', '\0'};
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

    /*
    .. Warn user for an unexpected case of encountering 0x00
    .. in the middle of source file.
    */
    #if 0
    if ( lxrbptr - lxrbuff < lxrsize - 1 ) {
      fprintf (stderr, "lxr warning : encountered byte 0x00 !!");
    }
    #endif

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
    .. Note : lxrBGNstatus, lxrENDstatus are available even before
    .. lxr_input (). Thus you can get boundary information without
    .. consuming.
    */

    /*
    .. In case next character is unknown, you have to expand/renew the
    .. the buffer. It's because you need to look ahead to look for
    .. boundary assertion patterns like abc$
    */
    if ( lxrbptr[1] == '\0' && lxrEOF == NULL )
      lxr_buffer_update ();

    if (*lxrbptr)
      return (int) (lxrprevchar = *lxrbptr++);
      
    /*
    .. return '\0' (equivalent to EOF) without consuming, so you can
    .. lxr_input () any number of times, each time returning '\0'.
    .. Note : byte 0x00 is not expected in the source code. So
    .. encountering it might cause unexpected behaviour. !!
    */
    if (lxrbptr [1] == '\0')
      return '\0';                                           /* EOF */

    return  
      (int) (lxrprevchar = *lxrbptr++);       /* unexpected 0x00 !! */

    /* LXR_BDRY (); */

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


static short check [918] = {
    0,

    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  317,  317,  317,  317,
  317,  317,  317,  317,  317,  317,  317,  317,  317,  317,
  317,  317,  317,  317,  317,  317,  317,  317,  317,  317,

  317,  317,  317,  317,  317,  317,  317,  317,  317,  317,
  317,  317,  317,  317,  317,  317,  317,  317,  317,  317,
  317,  317,  317,  317,  317,  317,  317,  317,  317,  317,
  317,  317,  317,  317,  317,  317,  317,  317,  317,  317,
  317,  317,  317,  317,  317,  317,  317,  317,  317,  317,
  317,  317,  317,  311,  311,  311,  311,  311,  311,  311,
  311,  311,   10,  311,  311,  311,  311,  311,  311,  311,
  311,  311,  311,  311,  311,  311,  311,  311,  311,  311,
  311,  311,  311,  311,  311,  311,  311,  311,  311,  311,
  311,  311,  311,  311,  311,  311,  311,  311,  311,  311,

  311,  311,  311,  311,  311,  311,  311,  311,  311,  311,
  311,  311,  311,  311,  311,  311,  311,  311,  311,  311,
  311,  311,  311,  311,  311,  311,  311,  311,  311,  311,
  107,  107,  107,  107,  107,  107,  107,  107,  107,  117,
  107,  107,  107,  107,  107,  107,  107,  107,  107,  107,
  107,  107,  107,  107,  107,  107,  107,  107,  107,  107,
  107,  107,  107,  107,  107,  107,  107,  107,  107,  107,
  107,  107,  107,  107,  107,  107,  107,  107,  107,  107,
  107,  107,  107,  107,  107,  107,  107,  107,  107,  107,
  107,  107,  107,  107,  107,  107,  107,  107,  107,  107,

  107,  107,  107,  107,  107,  107,  107,   48,   48,  133,
   48,   48,   48,   48,   48,  114,  118,   48,   48,   48,
  120,   48,   48,  119,  121,  122,  123,  114,  124,  125,
  119,  126,  127,  122,  128,  129,  130,   48,  131,  132,
  134,  133,  135,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,  136,  137,  138,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
  139,  140,  141,  143,  143,  144,  145,  146,  148,  149,
  150,  151,  152,  143,  146,  143,  147,  147,  147,  134,

  147,  147,  153,  147,  147,  147,  154,  155,  156,  157,
  147,  158,  159,  160,  161,  162,  163,  162,  164,  165,
  166,  167,  168,  169,  170,  171,  172,  173,  161,  167,
  174,  175,  176,  177,  178,  179,  179,  180,  181,  182,
  183,  184,  185,  186,  187,  188,  186,  189,  190,  191,
  192,  193,  194,  195,  196,  197,  198,  199,  200,  191,
  191,  201,  202,  203,  204,  205,  206,  207,  208,  209,
  210,  211,  212,  213,  214,  213,  215,  216,  212,  217,
  218,  219,  220,  213,  212,  221,  222,  223,  224,  225,
  226,  227,  228,  229,  230,  231,  232,  233,  234,  235,

  236,  237,  238,  239,  240,  241,  242,  243,  244,  245,
  246,  247,  248,  249,  250,  251,  252,  253,  254,  255,
  256,  257,  258,  259,  260,  261,  262,  263,  264,  319,
  320,  321,  322,  323,  324,  325,  326,  327,  328,  329,
  330,  331,  332,  333,  334,  335,  336,  337,  338,  339,
  340,  341,  342,  343,  344,  345,  346,  347,  348,  349,
  350,  351,  352,  353,  354,  355,  356,  357,  142,   49,
  281,   49,  267,   49,   49,  282,  300,  285,   49,   49,
   49,  111,  270,  111,  142,  298,  111,  315,  113,  315,
   93,  115,  315,  282,  111,  111,   49,  268,   49,  286,

  315,  315,   93,  298,   49,   49,   49,   49,  308,  115,
   49,  113,  308,  308,   49,  268,   96,   54,   98,   49,
   49,   49,   49,   49,   54,  267,   99,  292,   49,  278,
   99,  278,   49,  278,  278,   99,   49,  308,  278,  278,
  278,  111,   96,   96,  111,  292,  111,  315,   71,  111,
  315,   97,  315,   54,   87,  315,   54,  288,  278,   97,
  271,  100,  100,  287,  278,  278,  278,  278,   53,   54,
   53,   55,   53,   53,   54,   55,   55,  287,  271,  278,
  278,  278,  278,  278,  112,  287,  112,  106,  112,  112,
  297,  106,  278,  112,  316,  112,  316,   53,  316,  316,

   55,  104,  275,  316,  297,  316,   53,  299,  284,   53,
   94,   50,  297,  112,   86,   50,   50,  102,  102,  112,
  112,  112,  112,  316,  275,  299,  284,   53,  103,  316,
  316,  316,  316,   72,  112,  112,  112,  112,  112,  272,
   50,  104,  108,  272,  316,  316,  316,  316,  316,   51,
   51,  273,   50,  296,   51,   51,   50,  296,  296,   51,
  274,  301,  296,  116,  295,  272,  295,  302,  295,  295,
   50,   -1,   -1,  295,   -1,   -1,   -1,   51,   50,   51,
  296,   -1,  296,   58,   -1,   58,   -1,   58,   58,   -1,
   -1,   51,   -1,  295,   -1,   51,   57,   59,   57,   59,

   57,   57,  295,   59,   51,  295,   56,  296,   56,   51,
   56,   56,   58,   -1,   -1,   -1,   -1,   51,  295,   51,
   -1,   58,   92,  295,   58,   57,   92,   92,   -1,   -1,
   -1,   -1,   59,   -1,   57,   56,   59,   57,   -1,   -1,
  101,  307,   58,   -1,   56,  307,  307,   56,  307,   92,
  101,   92,  101,  101,   -1,   57,   -1,   -1,   59,   -1,
   -1,   -1,   -1,   -1,   -1,   56,  307,   -1,   -1,   -1,
  307,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1
};

static short next [918] = {
  265,

   50,  314,  186,  168,   51,   50,  107,   96,  106,  265,
  265,  265,  106,  265,  110,  105,  314,  314,  314,   94,
  100,   93,   88,   89,   98,   97,   85,   92,   99,   50,
   86,   82,  101,   87,  102,  265,  265,  265,  265,  265,
  265,   48,  265,  265,  265,   48,   90,   91,  103,  147,
  198,  223,  212,  122,  213,  199,  265,  134,  265,  217,
  265,  265,  265,  265,  192,  143,  139,  133,  176,  265,
  265,  265,   83,  104,   84,   95,  317,  317,  317,  317,
  317,  317,  317,  317,  317,    2,  317,  317,  317,  317,
  317,  317,  317,  317,  317,  317,  317,  317,  317,  317,

  317,  317,  317,  317,  317,  317,  317,  317,  317,  317,
  317,  317,  317,  317,  317,  317,  317,  317,  317,  317,
  317,  317,  317,  317,  317,  317,  317,  317,  317,  317,
  317,  317,  317,  317,  317,  317,  317,  317,  317,  317,
  317,  317,  317,  317,  317,  317,  317,  317,  317,  317,
  317,  317,  317,  311,  311,  315,  311,  311,  311,  311,
  311,  311,  174,  311,  311,  311,  311,  311,   52,  311,
  311,  311,  311,  311,  311,  311,  311,  311,  311,  311,
  311,  311,  311,  311,  311,  311,  311,  311,  311,  311,
  311,  311,  311,  311,  311,  311,  311,  311,  311,  311,

  311,  311,  311,  311,  311,  311,  311,  311,  311,  311,
  311,  311,  311,  311,  311,  311,  311,  311,  311,  311,
  311,  311,  311,  311,  311,  311,  311,  311,  311,  311,
  109,  109,  111,  109,  109,  109,  109,   59,  109,   43,
  109,  109,  109,  109,  109,  109,  109,  109,  109,  109,
  109,  109,  109,  109,  109,  109,  109,  109,  109,  109,
  109,  109,  109,  109,  109,  109,  109,  109,  109,  109,
  109,  109,  109,  109,  109,  109,  109,  109,  109,  109,
  109,  109,  109,  109,  109,  109,  109,  109,  109,  109,
  109,  109,  109,  109,  109,  109,  109,  109,  109,  109,

  109,  109,  109,  109,  109,  109,  109,  265,  265,  318,
  265,  265,  265,  265,  109,  120,   41,  265,  265,  265,
   37,  265,  313,  178,   22,  195,   44,  253,   13,   46,
   20,    4,   30,   10,   21,   36,   34,  265,   42,   47,
   18,  167,  329,  265,  265,  265,  265,  265,  265,  265,
  265,  265,  265,  265,  135,  136,  137,  265,  265,  265,
  265,  265,  265,  265,  265,  265,  265,  265,  265,  265,
  265,  265,  265,  265,  265,  265,  265,  265,  265,  265,
  138,  114,  252,  200,  142,  140,  144,  145,  132,  148,
  149,  150,  151,  161,  157,  220,  146,  235,  257,  119,

  160,  264,  131,  245,  210,  234,  328,  153,  326,  156,
  152,  155,  158,  159,  154,  130,  335,  177,  163,  164,
  165,  238,  162,  355,  348,  333,  129,  172,  187,  166,
  334,  332,  173,  175,  171,  356,  215,   45,  180,  352,
  320,  346,  184,  185,  183,  342,  347,  188,  189,  250,
  191,  182,  193,  194,  197,   17,  350,  196,  321,  190,
  324,  181,  201,  202,  203,  204,  205,  206,  207,  208,
  209,  179,  354,  170,  169,  236,  214,  128,  247,  216,
  127,  218,  219,  241,  211,  126,  221,  222,  319,  125,
  225,  226,  227,  228,  229,  230,  231,  232,  233,  224,

  124,  123,  322,  237,  345,  240,  239,  242,  243,  244,
  121,  351,  246,  248,  249,  118,  340,   38,  251,  254,
  255,  256,  117,  258,  259,  260,  261,  262,  263,   40,
   29,  341,   32,  339,  323,   39,  325,   28,  327,   31,
   35,  330,  331,   19,  353,   33,   26,  336,  337,   24,
   27,   25,  343,   23,   14,  344,   15,   16,   12,    3,
  349,    6,    9,   11,  357,    8,    7,    5,  338,   49,
  279,   49,  115,   49,   49,  293,  303,  289,   49,  282,
   49,  109,  275,  109,  141,  309,  109,  311,  109,  311,
   76,  269,  311,  293,  109,  109,  267,  280,   49,  290,

  311,  311,   68,  309,   49,   49,   49,   49,  308,  269,
  272,  116,  308,  308,  271,  280,   74,  306,   65,   49,
   49,   49,   49,   49,  298,  115,    1,  305,  270,  278,
  317,  278,  282,  278,  278,   66,  271,  308,  278,  268,
  278,  109,   64,   75,  109,  305,  109,  311,   61,  112,
  311,   73,  311,  306,   80,  316,  306,  290,  278,   63,
  273,   67,   84,  290,  278,  278,  278,  278,   53,  298,
  310,   55,   53,   53,  306,   55,   55,  289,  274,  278,
  278,  278,  278,  278,  109,  290,  109,  106,  109,  109,
  304,  106,  268,  109,  311,  109,  311,   53,  311,  311,

   55,   70,  276,  311,  303,  311,  310,  301,  286,  310,
   81,   50,  304,  109,   91,   50,   50,   79,   71,  109,
  109,  109,  109,  311,  276,  302,  288,  310,   69,  311,
  311,  311,  311,   62,  109,  109,  109,  109,  109,  275,
   50,   77,   60,  276,  311,  311,  311,  311,  311,  281,
  283,  276,  300,  296,  283,  296,  299,  296,  296,  307,
  276,  304,  307,  109,   54,  276,  294,  304,   54,   54,
  297,    0,    0,  292,    0,    0,    0,  295,  299,  296,
  295,    0,  296,   58,    0,  266,    0,   58,   58,    0,
    0,  285,    0,   54,    0,  284,   57,  109,  277,   59,

   57,   57,  294,   59,  307,  294,   56,  307,  291,  287,
   56,   56,   58,    0,    0,    0,    0,  284,  292,  281,
    0,  266,   54,  294,  266,   57,   54,   54,    0,    0,
    0,    0,  116,    0,  277,   56,  116,  277,    0,    0,
   83,   53,  266,    0,  291,   53,   53,  291,  312,  108,
   90,   54,   72,   78,    0,  277,    0,    0,  113,    0,
    0,    0,    0,    0,    0,  291,  312,    0,    0,    0,
   53,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,

    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0
};

static short base [358] = {
    0,

    0,    0,    0,    0,    0,    0,    0,    0,    0,   95,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  308,  569,  711,
  750,    0,  668,  615,  671,  806,  796,  783,  791,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  615,  700,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  680,  621,    0,    0,    0,
    0,  822,  569,  677,    0,  609,  626,  585,  602,  628,

  820,  684,  695,  668,    0,  679,  231,  715,    0,    0,
  580,  684,  582,  265,  584,  757,  169,  247,  264,  255,
  260,  271,  265,  268,  270,  273,  276,  279,  281,  283,
  286,  290,  280,  338,  288,  301,  301,  293,  310,  320,
  328,  513,  327,  330,  329,  328,  361,  339,  337,  329,
  324,  390,  345,  340,  343,  348,  347,  357,  351,  359,
  364,  358,  362,  357,  364,  363,  364,  360,  356,  359,
  364,  367,  370,  379,  365,  376,  383,  377,  370,  371,
  374,  380,  388,  391,  380,  384,  377,  388,  383,  382,
  394,  396,  384,  402,  451,  388,  393,  389,  395,  396,

  407,  397,  398,  414,  416,  414,  410,  402,  419,  404,
  410,  422,  414,  413,  419,  416,  417,  428,  415,  425,
  435,  432,  423,  426,  439,  438,  429,  433,  444,  441,
  445,  442,  433,  442,  437,  433,  437,  440,  436,  450,
  439,  440,  453,  444,  447,  456,  461,  446,  448,  457,
  461,  454,  515,  459,  456,  460,  459,  458,  473,  463,
  468,  471,  477,  468,    0,    0,  562,  590,    0,  523,
  619,  698,  710,  701,  657,    0,    0,  629,    0,    0,
  543,  568,    0,  667,  536,  558,  618,  598,    0,    0,
    0,  620,    0,    0,  764,  753,  645,  578,  666,  535,

  720,  708,    0,    0,    0,    0,  841,  608,    0,    0,
  154,    0,    0,    0,  586,  694,   77,    0,  470,  464,
  467,  471,  469,  467,  483,  479,  485,  481,  537,  486,
  482,  485,  489,  485,  492,  493,  493,  487,  488,  548,
  485,  500,  487,  493,  491,  490,  493,  504,  497,  494,
  497,  496,  509,  499,  511,  500,  513
};

static short accept [358] = {
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

  101,  102,  103,  104,  105,  106,  107,    0,    0,  107,
    0,    0,    0,   48,    0,    0,   48,   48,   48,   48,
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
   48,   48,   48,   48,   48,   58,    0,    0,    0,   49,
   49,   49,   49,   49,   49,   49,   57,    0,    0,    0,
    0,    0,   51,   51,   51,   51,   51,   51,   51,   51,
   56,    0,    0,   55,   55,    0,   50,    0,   50,   50,

   50,   50,   50,   50,    0,   54,    0,   54,    0,   53,
    0,    0,    0,  107,    0,    0,    0,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48
};

static short def [358] = {
   -1,

  302,    1,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   -1,  142,   51,
  315,    2,   54,  295,  295,   55,  308,   53,  268,   52,
   60,   61,   62,   63,   64,   65,   66,   67,   68,   69,
   98,  103,   70,   73,   74,   75,   76,   77,   78,   79,
   80,   81,   82,   83,   84,   94,   71,   85,   88,   89,
   90,  292,  289,  288,   91,  287,  102,  286,  285,  303,

  280,  284,   86,  106,   95,  299,   -1,   72,  107,  311,
  112,  281,   93,   48,   58,  301,   48,   48,   48,   48,
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
   48,   48,   48,   48,   48,  105,  278,   57,   58,  272,
  270,  297,  108,  273,   97,  266,  276,   49,  278,   57,
   49,   56,   51,  104,  287,  275,  305,   87,  287,  277,
  290,   55,   56,  291,   50,   56,   96,  308,  100,  297,

  274,  116,  297,  294,   55,  304,  115,   54,  308,  306,
  317,  307,  311,  310,  316,  112,    0,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48,   48,   48,   48,
   48,   48,   48,   48,   48,   48,   48
};

static unsigned char meta [77] = {
    0,

    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0
};

static unsigned char class [256] = {
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
};/*
.. table based tokenizer. Assumes,
.. (a) state '0' is the starting dfa.
.. (b) There is no zero-length tokens,
.. (c) maximum depth of 1 for "def" (fallback) chaining.
.. (d) Doesn't use meta class.
.. (e) Token value '0' : rejected. No substring matched
.. (f) accept value in [1, ntokens] for accepted tokens
*/

static char lxrholdchar = '\0';

int lxr_lex () {
  do {

    *lxrstrt = lxrholdchar;

    int s = 0,                                    /* state iterator */
      acc_token = 0,                         /* last accepted token */
      acc_len = 0,             /* length of the last accepted state */
      len = 0;                   /* consumed length for the current */
      LXR_BDRY ();

    /*
    .. fixme : copy the content of lxr_input ()
    */
    while ( s != -1 )  {
      int c = lxr_input (), ec = class [c];
      if ( accept [s] ) {
        acc_len = len;
        acc_token = accept [s];
      }
      ++len;

      /* dfa transition by c */
      while ( s != -1 && check [ base [s] + ec ] != s ) {
        /* note : no meta class as of now */
        s = def [s];
      }
      if ( s != -1 )
        s = next [base[s] + ec];
    }

    /*
    .. Update holding character & it's location.
    */
    char * lxrholdloc = lxrstrt + (acc_len + 1);
    lxrholdchar = *lxrholdloc;
    *lxrholdloc = '\0';

    /*
    .. handling accept/reject. In case of accepting, corresponding
    .. action snippet will be executed. In case of reject (unknown,
    .. pattern) lexer will simply consume without any warning. If you
    .. don't want to go unnoticed in such cases, you can add an "all
    .. catch" regex (.) in the last line of the rule section after all
    .. the expected patterns are defined. A sample lex snippet is
    ,, given below
    ..
    .. [_a-zA-Z][_a-zA-Z0-9]*                   { return (STRING);   }
    .. .                                        { return (WARNING);  }
    */

    do {   /* Just used a newer block to avoid clash of identifiers */
      switch ( acc_token ) {

      case 0 :
        printf ("%s", lxrstrt);
    break;
      case 1 :
        printf ("%s", lxrstrt);
    break;
      case 2 :
        printf ("%s", lxrstrt);
    break;
      case 3 :
        printf ("%s", lxrstrt);
    break;
      case 4 :
        printf ("%s", lxrstrt);
    break;
      case 5 :
        printf ("%s", lxrstrt);
    break;
      case 6 :
        printf ("%s", lxrstrt);
    break;
      case 7 :
        printf ("%s", lxrstrt);
    break;
      case 8 :
        printf ("%s", lxrstrt);
    break;
      case 9 :
        printf ("%s", lxrstrt);
    break;
      case 10 :
        printf ("%s", lxrstrt);
    break;
      case 11 :
        printf ("%s", lxrstrt);
    break;
      case 12 :
        printf ("%s", lxrstrt);
    break;
      case 13 :
        printf ("%s", lxrstrt);
    break;
      case 14 :
        printf ("%s", lxrstrt);
    break;
      case 15 :
        printf ("%s", lxrstrt);
    break;
      case 16 :
        printf ("%s", lxrstrt);
    break;
      case 17 :
        printf ("%s", lxrstrt);
    break;
      case 18 :
        printf ("%s", lxrstrt);
    break;
      case 19 :
        printf ("%s", lxrstrt);
    break;
      case 20 :
        printf ("%s", lxrstrt);
    break;
      case 21 :
        printf ("%s", lxrstrt);
    break;
      case 22 :
        printf ("%s", lxrstrt);
    break;
      case 23 :
        printf ("%s", lxrstrt);
    break;
      case 24 :
        printf ("%s", lxrstrt);
    break;
      case 25 :
        printf ("%s", lxrstrt);
    break;
      case 26 :
        printf ("%s", lxrstrt);
    break;
      case 27 :
        printf ("%s", lxrstrt);
    break;
      case 28 :
        printf ("%s", lxrstrt);
    break;
      case 29 :
        printf ("%s", lxrstrt);
    break;
      case 30 :
        printf ("%s", lxrstrt);
    break;
      case 31 :
        printf ("%s", lxrstrt);
    break;
      case 32 :
        printf ("%s", lxrstrt);
    break;
      case 33 :
        printf ("%s", lxrstrt);
    break;
      case 34 :
        printf ("%s", lxrstrt);
    break;
      case 35 :
        printf ("%s", lxrstrt);
    break;
      case 36 :
        printf ("%s", lxrstrt);
    break;
      case 37 :
        printf ("%s", lxrstrt);
    break;
      case 38 :
        printf ("%s", lxrstrt);
    break;
      case 39 :
        printf ("%s", lxrstrt);
    break;
      case 40 :
        printf ("%s", lxrstrt);
    break;
      case 41 :
        printf ("%s", lxrstrt);
    break;
      case 42 :
        printf ("%s", lxrstrt);
    break;
      case 43 :
        printf ("%s", lxrstrt);
    break;
      case 44 :
        printf ("%s", lxrstrt);
    break;
      case 45 :
        printf ("%s", lxrstrt);
    break;
      case 46 :
        printf ("%s", lxrstrt);
    break;
      case 47 :
        printf ("%s", lxrstrt);
    break;
      case 48 :
        printf ("%s", lxrstrt);
    break;
      case 49 :
        printf ("%s", lxrstrt);
    break;
      case 50 :
        printf ("%s", lxrstrt);
    break;
      case 51 :
        printf ("%s", lxrstrt);
    break;
      case 52 :
        printf ("%s", lxrstrt);
    break;
      case 53 :
        printf ("%s", lxrstrt);
    break;
      case 54 :
        printf ("%s", lxrstrt);
    break;
      case 55 :
        printf ("%s", lxrstrt);
    break;
      case 56 :
        printf ("%s", lxrstrt);
    break;
      case 57 :
        printf ("%s", lxrstrt);
    break;
      case 58 :
        printf ("%s", lxrstrt);
    break;
      case 59 :
        printf ("%s", lxrstrt);
    break;
      case 60 :
        printf ("%s", lxrstrt);
    break;
      case 61 :
        printf ("%s", lxrstrt);
    break;
      case 62 :
        printf ("%s", lxrstrt);
    break;
      case 63 :
        printf ("%s", lxrstrt);
    break;
      case 64 :
        printf ("%s", lxrstrt);
    break;
      case 65 :
        printf ("%s", lxrstrt);
    break;
      case 66 :
        printf ("%s", lxrstrt);
    break;
      case 67 :
        printf ("%s", lxrstrt);
    break;
      case 68 :
        printf ("%s", lxrstrt);
    break;
      case 69 :
        printf ("%s", lxrstrt);
    break;
      case 70 :
        printf ("%s", lxrstrt);
    break;
      case 71 :
        printf ("%s", lxrstrt);
    break;
      case 72 :
        printf ("%s", lxrstrt);
    break;
      case 73 :
        printf ("%s", lxrstrt);
    break;
      case 74 :
        printf ("%s", lxrstrt);
    break;
      case 75 :
        printf ("%s", lxrstrt);
    break;
      case 76 :
        printf ("%s", lxrstrt);
    break;
      case 77 :
        printf ("%s", lxrstrt);
    break;
      case 78 :
        printf ("%s", lxrstrt);
    break;
      case 79 :
        printf ("%s", lxrstrt);
    break;
      case 80 :
        printf ("%s", lxrstrt);
    break;
      case 81 :
        printf ("%s", lxrstrt);
    break;
      case 82 :
        printf ("%s", lxrstrt);
    break;
      case 83 :
        printf ("%s", lxrstrt);
    break;
      case 84 :
        printf ("%s", lxrstrt);
    break;
      case 85 :
        printf ("%s", lxrstrt);
    break;
      case 86 :
        printf ("%s", lxrstrt);
    break;
      case 87 :
        printf ("%s", lxrstrt);
    break;
      case 88 :
        printf ("%s", lxrstrt);
    break;
      case 89 :
        printf ("%s", lxrstrt);
    break;
      case 90 :
        printf ("%s", lxrstrt);
    break;
      case 91 :
        printf ("%s", lxrstrt);
    break;
      case 92 :
        printf ("%s", lxrstrt);
    break;
      case 93 :
        printf ("%s", lxrstrt);
    break;
      case 94 :
        printf ("%s", lxrstrt);
    break;
      case 95 :
        printf ("%s", lxrstrt);
    break;
      case 96 :
        printf ("%s", lxrstrt);
    break;
      case 97 :
        printf ("%s", lxrstrt);
    break;
      case 98 :
        printf ("%s", lxrstrt);
    break;
      case 99 :
        printf ("%s", lxrstrt);
    break;
      case 100 :
        printf ("%s", lxrstrt);
    break;
      case 101 :
        printf ("%s", lxrstrt);
    break;
      case 102 :
        printf ("%s", lxrstrt);
    break;
      case 103 :
        printf ("%s", lxrstrt);
    break;
      case 104 :
        printf ("%s", lxrstrt);
    break;
      case 105 :
        printf ("%s", lxrstrt);
    break;
      case 106 :
        printf ("%s", lxrstrt);
    break;

        default :                                /* Unknown pattern */
      }
    } while (0);
    lxrbptr = lxrstrt = lxrholdloc;

  } while (lxrholdchar != '\0');

  return EOF;
}

int main () {
  lxr_lex ();
  return 0;
}
