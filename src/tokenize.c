/*
.. table based tokenizer. Assumes,
.. (a) state '0' is the starting dfa.
.. (b) There is no zero-length tokens,
.. (c) maximum depth of 1 for "def" (fallback) chaining.
.. (d) Doesn't use meta class.
.. (e) Token value '0' : rejected. No substring matched
.. (f) accept value in [1, ntokens] for accepted tokens
*/

int lxr_lex () {
  int c = 0, ec;
  while (1) {

    int s = 0,                                    /* state iterator */
      acc_token = 0,                         /* last accepted token */
      LXR_BDRY ();
      acc_len = 0,             /* length of the last accepted state */
      len = 0;                   /* consumed length for the current */

      /*
      .. fixme : copy the content of lxr_input ()
      */
      while ( s != -1 )  {
        c = lxr_input ();
        ec = class [c];

        if ( accept [s] ) {
          acc_len = len;
          acc_token = accept [s];
        }
        ++len;
        
        /* dfa transition by c */
        while ( s != -1 && check [ base [s] + ec ] != s ) {
          s = def [s];
        }
        if ( s != -1 )
          s = next [base[s] + c];
      }

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

    do {   /* Just used a newer block to avoid class of identifiers */
      switch ( acc_token ) {
        /*%%*/
        default :
          lxrbptr = ++lxrstrt;                   /* Unknown pattern */
      }
    } while (0);
  }

  return EOF;
}
