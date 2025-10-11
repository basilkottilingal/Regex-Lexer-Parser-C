

/*
.. table based tokenizer. Assumes,
.. (a) state '0' : REJECT
..     state '1' is the starting dfa.
..     state '2' is the starting dfa (BOL)
.. (b) There is no zero-length tokens,
.. (c) maximum depth of 1 for "def" (fallback) chaining.
.. (d) Doesn't use meta class.
.. (e) Token value '0' : rejected. No substring matched
.. (f) accept value in [1, ntokens] for accepted tokens
*/

#define lxr_max_depth                        1
#define lxr_dead                             0
#define lxr_not_rejected(s)                  (s)          /* s != 0 */
#ifndef lxr_state_stack_size 
#define lxr_state_stack_size                 128
#endif

#define lxr_tokenizer_init()                                         \
    do {                                                             \
      yytext [yyleng] = lxr_hold_char;                               \
      yytext += lxr_bptr - lxr_start;                                \
      state = 1 + (yytext [-1] == '\n');                             \
      acc_token = acc_len = len = 0;                                 \
      lxr_start = lxr_bptr;                                          \
      idx = lxr_state_stack_size;                                    \
    } while (0) 

/*
.. The main lexer function. Returns 0, when EOF is encountered. So,
.. don't use return value 0 inside any action.
*/
int lxr_lex () {

  int state, class, acc_token, acc_len, len, idx, tkn;
  int acc_len_old, acc_token_old, state_old;
  static int states [lxr_state_stack_size];

  lxr_tokenizer_init();

  do {                  /* Loop looking the longest token until EOF */

    /* in case to back up */
    acc_len_old = acc_len;
    acc_token_old = acc_token;
    state_old = state;
    
    do {                            /* Transition loop until reject */
      class = (int) *lxr_bptr++;

      /*
      .. find the transition corresponding to the class using check/
      .. next tables and if not found in [base, base + nclass), use
      .. the fallback
      .. Note: (a) No meta class as of now. (b) assumes transition
      .. is DEAD if number of fallbacks reaches lxr_max_depth
      */
      int depth = 0;
      while ( lxr_not_rejected (state) && 
        ((int) lxr_check [lxr_base [state] + class] != state) ) {
        state = (depth++ == lxr_max_depth) ? lxr_dead : 
          (int) lxr_def [state];
      }

      /*
      .. Keep stack of states
      */
      if ( lxr_not_rejected (state) )
        state = (int) lxr_next [lxr_base[state] + class];
      states [--idx] = state; 
    } while ( lxr_not_rejected (state) && idx );

    len = (int) (lxr_bptr - lxr_start);
    while (idx < lxr_state_stack_size) {
      /*
      .. (a) Longest token (b) In case of clash use the first token
      .. defined in the lexer file
      */
      if ( (tkn = lxr_accept [states [idx++]]) ) {
        acc_len = len; acc_token = tkn;
        break;
      }
      len--;
    }

    if (lxr_not_rejected (state)) {
      idx = lxr_state_stack_size;
      continue;
    }

    /*
    .. Put the reading pointer at the last accepted location.
    .. Update with the new holding character.
    */
    yyleng = acc_len ? acc_len : (acc_len = 1);
    lxr_bptr = lxr_start + acc_len;
    lxr_hold_char = yytext [acc_len];
    yytext [acc_len] = '\0';

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

        /*% replace this line with case <token> : <action>  break; %*/

        case lxr_eof_accept :
          printf ("\nEOF");
          lxr_bptr = lxr_start;
          yyleng = 0;
          return 0;

        case lxr_eob_accept :
          printf ("\nEOB");
          lxr_bptr --;
          lxr_buffer_update ();
          break;

        default :                                /* Unknown pattern */

      }
    } while (0);

    if (acc_token == lxr_eob_accept) {
      state = (idx == lxr_state_stack_size) ? 
        state_old : states [idx];
      acc_len = acc_len_old;
      acc_token = acc_token_old;                      /* backing up */
      continue; /* continue from where we stopped lexing bcz of eob */
    }

    lxr_tokenizer_init ();             /* start reading a new token */

  } while (1);

  return EOF;                     /* The code will never reach here */
}

int lxr_input () {
  if (*lxr_bptr == lxr_eob_class)
    lxr_buffer_update ();
  if (*lxr_bptr == lxr_eof_class)
    return EOF;
  size_t idx = lxr_bptr++ - lxr_start;
  return (int) (unsigned char) 
   ( ((size_t) yyleng == idx) ? lxr_hold_char : yytext [idx] );
}

static void lxr_buffer_update () {

  if (lxr_in == NULL) lxr_in = stdin;

  if (lxr_bptr [0] != lxr_eob_class ||
      lxr_bptr [1] != lxr_eob_class)
  {
    fprintf (stderr, "lxr_buffer_update () : internal check failed");
    fprintf (stderr, "[%d, %d]", lxr_bptr [0], lxr_bptr [1]);
    exit (-1);
  }
    
  /*
  .. "non_parsed" : Bytes already consumed by automaton but yet
  .. to be accepted. These bytes will be copied to the new buffer.
  .. fixme : may reallocate if non_parsed is > 50 % of buffer.
  .. (as of now, reallocate if non_parsed == 100% of buffer)
  */
  size_t size = lxr_size, non_parsed = lxr_bptr - lxr_start;

  lxr_stack * s = lxr_current;
  if (s == NULL || (yytext - s->bytes) > (size_t) 1 ) {

    while ( size <= non_parsed )
      size *= 2;

    s = lxr_alloc (sizeof (size));
    if ( s == NULL ) {
      fprintf (stderr, "lxr_alloc failed");
      exit (-1);
    }
    * s = (lxr_stack) {
      .size  = size,
      .bytes = lxr_alloc (size + 2),
      .class = lxr_alloc (size + 2),
      .next  = lxr_current
    };
    if ( s->bytes == NULL || s->class == NULL ) {
      fprintf (stderr, "lxr_alloc failed");
      exit (-1);
    }
    memcpy (s->bytes, & yytext [-1], non_parsed + 1);
    memcpy (s->class, lxr_start,     non_parsed);

    lxr_current = s; 
  }
  else {
    size = s->size * 2;
    s->bytes = lxr_realloc (s->bytes, size + 2);
    s->class = lxr_realloc (s->class, size + 2);
    s->size  = size;
    if ( s->bytes == NULL || s->class == NULL ) {
      fprintf (stderr, "lxr_realloc failed");
      exit (-1);
    }
  }

  yytext = s->bytes + 1;
  lxr_start = s->class;
  lxr_bptr = lxr_start + non_parsed;
  lxr_hold_char = *yytext;
    
  size_t bytes =
    fread ( & yytext [non_parsed], 1, lxr_size - non_parsed, lxr_in );

  unsigned char end_class = lxr_eob_class;
  if (bytes < lxr_size - non_parsed) {
    if (!feof (lxr_in)) {
      fprintf (stderr, "lxr buffer : fread failed !!");
      exit (-1);
    }
    end_class = lxr_eof_class;
  }
  unsigned char * byte = & ((unsigned char *) yytext) [non_parsed] ;
  for (size_t i=0; i<bytes; ++i)
    lxr_bptr [i] = lxr_class [ *byte++];
  lxr_bptr [bytes] = lxr_bptr [bytes + 1] = end_class;
  yytext   [bytes + non_parsed] = '\0';

}

