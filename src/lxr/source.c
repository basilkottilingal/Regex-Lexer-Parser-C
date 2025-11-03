

/*
.. ---------------------------- Lexer --------------------------------
.. This is a generated file for table based lexical analysis. The file
.. contains (a) functions and macros related to reading source code
.. and running action when a lexicon detected (b) compressed table for
.. table based lexical analysis.
*/

typedef struct lxr_buff_stack {
  char * bytes;
  size_t size;
  struct lxr_buff_stack * next; 
} lxr_buff_stack ;

static unsigned char
  lxr_dummy[3] = {lxr_eob_class, lxr_eob_class, lxr_eob_class};
static unsigned char * lxr_start = lxr_dummy;
static unsigned char * lxr_bptr  = lxr_dummy;
static unsigned char * lxr_class_buff = NULL;
static int lxr_class_buff_size = 0;

static char lxr_hold_char = '\0';

#define lxr_reset() do {                                             \
    lxr_start = lxr_bptr = lxr_dummy;                                \
    lxr_hold_char = '\0';                                            \
    yytext = lxr_sample + 1;                                         \
    yyleng = 0;                                                      \
  } while (0)

static lxr_buff_stack * lxr_buff_stack_current = NULL;

void lxr_source (const char * in) {
  if (lxr_in) {
    fprintf (stderr, "cannot change source file in the middle."
      "\nuse lxr_stack_push (source) to change source file."
      "\nalternatively use lxr_clear() + lxr_source()");
    exit (-1);
  }
  if (in == NULL || (lxr_in = fopen (in, "r")) == NULL) {
    fprintf (stderr, "cannot find lxr_infile %s", in ? in : "");
    fflush (stderr);
    exit (-1);
  }
  lxr_infile = strdup (in);
}

void lxr_read_bytes (const unsigned char * bytes, size_t len) {
}
  
#ifndef LXR_BUFF_SIZE
  #define LXR_BUFF_SIZE  1<<16       /* default buffer size : 16 kB */
#endif
static size_t lxr_size = LXR_BUFF_SIZE;

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

void lxr_clean () {
  /*
  .. Free all the memory blocks created for buffer.
  .. User required to run this at the end of the program
  */
  lxr_buff_stack * s = lxr_buff_stack_current, * next;
  while (s != NULL) {
    next = s->next;
    free (s->bytes);
    free (s);
    s = next;
  }
  lxr_buff_stack_current = NULL;

  if (lxr_class_buff) {
    free (lxr_class_buff);
    lxr_class_buff = NULL;
  }

  if (lxr_in)
    fclose (lxr_in);
  if (lxr_infile)
    free (lxr_infile);
  lxr_in = NULL;
  lxr_infile = NULL;
  lxr_reset ();
}
