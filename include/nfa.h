#ifndef _RGX_NFA_H_
#define _RGX_NFA_H_

  #include <stdlib.h>
  #include <stdio.h>
  #include <string.h>
  #include <assert.h>

  #include "regex.h"
  #include "allocator.h"
  #include "stack.h"

  int states_add        ( State * start, Stack * list, State *** buff );
  int states_at_start   ( State * nfa,   Stack * list, State *** buff );
  int states_transition ( Stack * from,  Stack * to,   State *** buff, int c );
  int nfa_info          ( State * start, char ** token );
  int states_bstack     ( Stack * list,  Stack * bits );

  #define RGXMATCH(_s_) (_s_)->flag

#endif
