/*
.. test case for nfa.
.. $ make clean  &&  make CFLAGS+="-DRGXLRG" test-hopcroft.tst
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"

int main () {
  /* char * rgxlist[] = { "(a|b)+0","(11*0+0)(0+1)*0*1*","(a*|a+)" }; */
  char * rgxlist[] = { "(11*0+0)(0+1)*0*1*" };
  const char txt[] = "11000110100101001010001001000111101010101001";
  char buff[60];

  for (int i=0; i<sizeof(rgxlist)/sizeof(rgxlist[0]); ++i) {
    char * rgx = rgxlist[i];
    DState * dfa[2] = {0};
    int status =  rgx_dfa (rgx, dfa); 
    switch (status) {
      case 0 :  
        printf("\n Hopcroft minimsation of rgx %s : ", rgx);
        printf ("|Q'| = |Q|"); break;
      case 1 :  
        printf("\n Hopcroft minimsation of rgx %s : ", rgx);
        printf ("|Q'| < |Q|"); break;
      default : printf ("\n unknown error");
    }
    if (dfa<=0) continue;
    /* Sample text */
    const char * source = txt;
    do {
      int m = rgx_dfa_match (dfa[1], source);
      if (m < 0) printf ("failed in match check %d", m);
      //source += m > 1 ? (m-1)
      if (m > 0) {
        m -= 1; buff[m] = '\0';
        if(m) memcpy (buff, source, m);
        printf ("\nRegex %s: found in txt \"%s\" (\"%s\")", rgx, buff, source);
      }
    } while (*++source);
  
  }
    
  /* free all memory blocks created */
  rgx_free();

}
