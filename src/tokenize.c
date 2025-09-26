/*
.. table based tokenizer. Assumes,
.. (a) state '0' is the starting dfa.
1
.. (c) maximum depth of 1 for "def" (fallback) chaining.
.. (d) Doesn't use meta class.
.. (e) Token value '0' : rejected. No substring matched
.. (f) accept value in [1, ntokens] for accepted tokens
*/

static char lxrholdchar = '\0';

int lxr_lex () {

  int isEOF = 0;

  do {

    *lxrstrt = lxrholdchar;       /* put back the holding character */

    int c, ec, s = 0,                             /* state iterator */
      acc_token = 0,                         /* last accepted token */
      acc_len = 0,             /* length of the last accepted state */
      len = 0;                   /* consumed length for the current */

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
      while ( s != -1 && check [ base [s] + ec ] != s ) {
        /* note : no meta class as of now */
        s = def [s];
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
        /*% replace this line with "case <token> : <action> break; %*/

        default :                                /* Unknown pattern */
          /*% if (isEOF) replace this line with any snippet reqd   %*/
      }
    } while (0);

    /* Place reading idx just after the last character of the token */
    lxrbptr = lxrstrt = lxrholdloc;

  } while (!isEOF);    /* Will stop when the first character is EOF */

  return EOF;
}
