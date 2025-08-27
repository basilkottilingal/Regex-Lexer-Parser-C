/*
.. test case for nfa.
.. $ make clean  &&  make CFLAGS+="-DRGXLRG" test-hopcroft.tst
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"

int main () {
  char * rgxlist[] = { "(a|b)+0","(11*0+0)(0+1)*0*1*","(a*|a+)" };
  const char txt[] = "aaccbqaabbaba01bcccd";
  char buff[60];

  for (int i=0; i<sizeof(rgxlist)/sizeof(rgxlist[0]); ++i) {
    char * rgx = rgxlist[i];
    DState * dfa;
    switch ( DFA_MINIMIZATION (rgx, &dfa) ) {
      case 0 :  
        printf("\n Hopcroft minimsation of rgx %s : ", rgx);
        printf ("|Q'| = |Q|"); break;
      case 1 :  
        printf("\n Hopcroft minimsation of rgx %s : ", rgx);
        printf ("|Q'| < |Q|"); break;
      default : printf ("unknown error");
    }
  }

  /* free all memory blocks created */
  rgx_free();
}
