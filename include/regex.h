#ifndef _RGX_H_
#define _RGX_H_
  #include "stack.h"
  /*
  .. Warning !!!
  .. Regex reading functions doesn't take care of all regex rules and 
  .. patterns. Regex rules here are enough to define patterns for JSON
  .. lexicons, C lexicons.
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
  .. (a) int rgx_match ( const char * rgx, const char * txt ) :
  ..      returns a integer value, say, val > 0 if the text, "txt" (or 
  ..      a starting substring of "txt") matches the regex pattern. 
  ..      The number of characters matched = val - 1.
  ..      If val = 0, then "txt" doesn't follow "rgx" pattern,
  ..      and val < 0 refers to some error.
  .. (b) int rgx_dfa ( const char * rgx, Dstate ** dfa );
  ..      For repeated match lookup of a regex "rgx", this function
  ..      will evaluate an equivalent form of  minimal DFA in *dfa.
  ..      The return value < 0 means an error.
  .. (c) int rgx_dfa_match ( DState * dfa, const char * txt);
  ..      Similar to "rgx_match", but uses minimal "dfa" for "rxg"
  .. (d) int rgx_free ();
  ..      Free allocated memory blocks.
  */
  int  rgx_match     ( /*const*/ char * rgx, const char * txt );
  int  rgx_dfa       ( /*const*/ char * rgx, DState ** dfa );
  int  rgx_dfa_match ( DState * dfa,     const char * txt);
  void rgx_free ();

  /*
  .. Lower level or internal api. Maybe used for debug
  .. (a) creates and NFA
  .. (b) see if "txt" matches regex "rgx" using the nfa created
  .. (c) Create a dfa for a list of "rgx", if the list
  ..      have the list of "rgx" with decreasing preference,
  ..      in case of multiple "rgx" accepts the "txt".
  */
  int  rgx_nfa       ( char   * rgx, State ** nfa, int itoken );
  int  rgx_nfa_match ( State  * nfa, const char * txt );
  int  rgx_list_dfa  ( Stack * rgx_list, DState ** dfa );

  enum RGXFLAGS {

    RGXEOE  = -1,   /* "End of expression" : Regex parsed successfully */
    RGXOOM  = -10,  /* "Out of Memory" : A stack ran out of space */
    RGXERR  = -11,  /* "Error" : Unknown/Non-implemented regex pattern*/
  };

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
