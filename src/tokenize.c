

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
#define lxr_not_rejected(s)                 (s)           /* s != 0 */

#define lxr_tokenizer_init() do {\
      lxr_bol_status = (yytext [-1] == '\n');                        \
      state = 1 + lxr_bol_status;                                    \
      acc_token = acc_len = len = 0;                                 \
      lxr_start = lxr_bptr;                                          \
      *lxr_start = lxr_hold_char;                                    \
      yytext += yyleng;                                              \
    }   while (0) 

/*
.. The main lexer function. Returns 0, when EOF is encountered. So,
.. don't use return value 0 inside any action.
*/
int lxr_lex () {

  int lxr_bol_status, state, class, acc_token, acc_len, len;
  lxr_tokenizer_init();

  do {                  /* Loop looking the longest token until EOF */

    do {                            /* Transition loop until reject */
      class = (int) *lxr_bptr++;
      len++;

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
      .. Update the most eligible token using the criteria
      .. (a) Longest token (b) In case of clash use the first token
      .. defined in the lexer file
      */
      if ( lxr_not_rejected (state) ) {
        state = (int) lxr_next [lxr_base[state] + class];
        if ( lxr_not_rejected (state) && lxr_accept [state] &&
          ( len > acc_len || (int) lxr_accept [state] < acc_token) )
        {
          acc_len = len;
          acc_token = (int) lxr_accept [state];
        }
      }

    } while ( lxr_not_rejected (state) ); 


    /*
    .. Put the reading pointer at the last accepted location.
    .. Update with the new holding character.
    */
    yyleng = acc_len = (acc_len ? acc_len : 1);
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
          return 0;

        case lxr_eob_accept :
          printf ("\nEOB");
          lxr_bptr --;
          lxr_buffer_update ();
          break;

        default :                                /* Unknown pattern */

      }
    } while (0);

    if (acc_token == lxr_eob_accept)
      continue; /* continue from where we stopped lexing bcz of eob */

    lxr_tokenizer_init ();             /* start reading a new token */

  } while (1);

  return EOF;                     /* The code will never reach here */
}

#undef lxr_dead
