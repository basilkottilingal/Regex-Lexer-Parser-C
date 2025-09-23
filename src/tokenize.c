/*
.. table based tokenizer. Assumes,
.. (a) state '0' is the starting dfa.
.. (b) There is no zero-length tokens,
.. (c) maximum depth of 1 for "def" (fallback) chaining.
.. (d) Doesn't use meta class.
.. (e) accept value '0' : not accepted,
.. (f) accept value in [1, ntokens] for accepted tokens
*/

int lxr_lex () {
  #define LEX
  int c = 0;
  while ( c != EOF () ) {
    int s = 0,  /* state iterator */
      acc_token = 0,    /* last accepted token */
      acc_len = 0,  /* length of the last accepted state */
      len = 0;  /* consumed length for the current */
    do {
    } while (  );
    /*%%*/
  }
}
