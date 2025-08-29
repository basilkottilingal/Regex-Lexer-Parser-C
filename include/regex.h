#ifndef _RGX_H_
#define _RGX_H_

  /*
  .. Warning !!!
  .. Regex reading functions doesn't take care of all regex rules and patterns.
  .. Regex rules here are enough to define patterns for JSON lexicons, C lexicons.
  ..
  .. In case, to expand regex reader to create an exhuastive set, read
  .. https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Regular_expressions/Cheatsheet
  ..
  .. If interested in implementing NFA and DFA, read
  .. https://swtch.com/~rsc/regexp/regexp1.html
  */

  /* 
  .. NFA and DFA State
  */
  typedef struct State {
    struct State ** out;
    int id, counter, ist, flag;
  } State ;  
  typedef struct DState DState;

  /*
  .. API
  .. (a) rgx_match (rgx, txt), see if "txt" matches regular expression "rgx".
  */
  //  int  rgx_match ( char * rgx, const char * txt );

  /*
  .. API
  .. (a) create an regex engine from a regex pattern.
  .. (b) See if a text contains the pattern. Return null in case not found.
  .. (c) destroy() all memory block, created.
  ..     Call @ the end of progam to clear all mem block.
  */
  int  rgx_nfa       ( char   * rgx, State ** nfa, int itoken );
  int  rgx_dfa       ( char   * rgx, DState ** dfa );
  int  rgx_nfa_match ( State  * nfa, const char * txt );
  int  rgx_dfa_match ( DState * dfa, const char * txt );
  void rgx_free      ( );

  // not yet complete
  DState *  FULL_DFA ( char * rgx );
  int  FULL_DFA_PATTERN ( DState * dfa, const char * txt );
  int  DFA_MINIMIZATION ( char * rgx, DState ** dfa);

  enum RGXFLAGS {

    RGXEOE  = -1,   /* EOE ("End of expression") : Regex parsed successfully */
    RGXOOM  = -10,  /* OOM ("Out of Memory") : A stack ran out of space */
    RGXERR  = -11,  /* ERR ("Error") : Unknown/Non-implemented regex pattern*/

    NFAEPS  = 256,  /* EPS ("epsilon") : Epsilon/Empty transition, or epsilon transition */
    NFAACC  = 257,  /* ACC ("accept")  : Accepting state, ( may be associated with a token name) */
    NFAERR  = -2,   /* ERR ("error")   : Flag Error while creating an NFA state*/
    NFAREJ  = -3,   /* REJ ("reject")  : May be used as a complementary to NFAACC */

  };

  /*
  .. Define RGXLRG if you expect large number of NFA/DFA counts, like language lexer
  .. Bit stacking, hash comparison, etc will be switched on
  */
  #ifdef RGXLRG
    #define RGXHSH
  #endif

  /*
  .. RGXSIZE is the default size of stack for rgx literals/NFA states/ etc
  .. If an implementation fail with RGXOOM flag, rerun with larger RGXSIZE
  */
  #ifndef RGXSIZE
    #define RGXSIZE 64
  #endif

  /* #define RGXNFA_CACHE_LIMIT 512 */

  #define   RGXOP(_c_)     ((_c_) | 256)
  #define ISRGXOP(_c_)     ((_c_) & 256)

  /* Lower level/Internal funtions. Maybe used for debug */
  int rgx_rpn       ( char * rgx, int ** rpn );
  int rgx_rpn_print ( int * rpn );

#endif
