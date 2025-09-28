

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

      if (lxrEOLstatus) {
        class = lxrEOLclass; lxrEOLstatus = 0;
        last_state = state;
      }
      else {
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

      while ( state != LXRDEAD && 
        ((int) lxr_check [lxr_base [state] + class] != state) ) {
        /*
        .. Note: (a) No meta class as of now. (b) assumes transition
        .. is DEAD if number of fallbacks reaches LXR_MAXDEPTH
        */
        state = (depth++ == LXR_MAXDEPTH) ? LXRDEAD : 
          (int) lxr_def [state];
      }

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
      .. We proceed further after EOL
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
        /*% replace this line with "case <token> : <action> break; %*/

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
