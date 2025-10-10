

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

#define LXR_MAXDEPTH         1
#define LXRDEAD              0
#define lxr_not_rejected(s)  (s)    /* s != 0 */

int lxr_lex () {
  int is_eof = 0;
    
  int lxr_bol_status = (lxr_bptr [-1] == '\n');

  do {                  /* Loop looking the longest token until EOF */

    lxr_start = lxr_bptr;
    *lxr_start = lxr_hold_char;   /* put back the holding character     */

    int uchar, class,       /* input character & it's class         */
      state = 1 + lxr_bol_status, /* start with 1 or 2 depending on BOL flag */
      last_state = LXRDEAD, /* if EOL transition failed, go back    */
      acc_token = 0,        /* last accepted token                  */
      acc_len = 0,          /* length of the last accepted state    */
      len = 0,              /* consumed length for the current      */
      depth,                /* depth of fallback                    */
      lxr_eol_status = 
        ( lxr_bptr == lxr_eof ) || ( *lxr_bptr == '\n' );

    do {                            /* Transition loop until reject */

      #if 0
      /*
      .. Before running the transition corresponding to byte input
      .. from the file, we do the EOL transition if the next byte is
      .. '\n' or has reached EOF
      */
      if (lxr_eol_status) {
        class = lxr_eol_class; lxr_eol_status = 0;
        last_state = state;
      }
      else {
        /*
        .. Consume the byte from file/buffer and update EOL status
        */
        if ( *lxr_bptr == '\0' && lxr_eof == NULL )
          lxr_buffer_update ();
        if (lxr_bptr == lxr_eof) {
          if (len == 0) is_eof = 1;
          break;
        }
        uchar = (unsigned char) *lxr_bptr++;
        len++;
        class = lxr_class [uchar];
        lxr_eol_status = ( lxr_bptr == lxr_eof ) || (*lxr_bptr == '\n');
      }
      #else

      class = *lxr_bptr ? lxr_class [*lxr_bptr++] :
        (lxr_bptr == lxr_eof) ? lxr_eof_class : lxr_eob_class; 
      len++;
      #endif

      /*
      .. find the transition corresponding to the class using check/
      .. next tables and if not found in [base, base + nclass), use
      .. the fallback
      */
      depth = 0;
      while ( lxr_not_rejected (state) && 
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
      if ( lxr_not_rejected (state) ) {
        state = (int) lxr_next [lxr_base[state] + class];
        if ( lxr_not_rejected (state) && lxr_accept [state] &&
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
      #if 0
      if (class == lxr_eol_class)
        state = last_state;
      #endif

    } while ( lxr_not_rejected (state) ); 


    /*
    .. Update with new holding character & it's location.
    */
    lxr_bptr = lxr_start + (acc_len ? acc_len : 1);
    lxr_hold_char = *lxr_bptr;
    *lxr_bptr = '\0';
    lxr_bol_status = (lxr_bptr [-1] == '\n');

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
          lxr_bptr --;
          lxr_bol_status = (lxr_start [-1] == '\n');
          return 0;
        case lxr_eob_accept :
          printf ("\nEOB");
          lxr_bptr --;
          lxr_bol_status = (lxr_start [-1] == '\n'); //fixme : segfault if the whole 
          lxr_buffer_update ();
          break;
        default :                                /* Unknown pattern */
      }
    } while (0);

  } while (!is_eof);    /* Will stop when the first character is EOF */

  return EOF;
}

#undef LXRDEAD
