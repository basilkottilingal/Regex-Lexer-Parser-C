/*
.. test case for nfa.
.. $ make test-nfa.s && ./test-nfa.s
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"

int main () {
  //char * rgx[] = { "(aa|b)|(a(a|b))|(bc*)|(bc+)|((a|b)+0)" };
  char * rgx[] = { "(aa|b)|(a(a|b))|(bc*)|(bc+)|(1?)|((a|b)+0)" };
  //char * rgx[] = { "aa|b", "a(a|b)", "bc*", "bc+", "(a|b)+0" };
  //char * rgx[] = { "aa|b", "a(a|b)", "bc*", "bc+", "1?", "(a|b)+0" };
  //char * rgx[] = { "1?" };
  const char txt[] = "aaccbqaabbaba01bcccd";
  char buff[60];
  DState * dfa[2] = {0} ;
  for (int i=0; i< sizeof (rgx) / sizeof (rgx[0]); ++i) {
    int status = rgx_dfa (rgx[i], dfa);
    if (status < 0) { 
      printf ("failed in creating DFA for rgx %s: %d", rgx[i], status); 
      break;
    }
  
    for (int j=0; j<1 + (status>0); ++j) {
      if (j)  printf ("\n\nUsing minimised DFA ");
      const char * source = txt;
      do {
        int m = rgx_dfa_match (dfa[j], source);
        if (m < 0) printf ("failed in match check %d", m); 
        //source += m > 1 ? (m-1) 
        if (m > 0) {
          m -= 1; buff[m] = '\0';
          if(m) memcpy (buff, source, m);
          printf ("\nRegex %s: found in txt \"%s\" (\"%s\")", rgx[i], buff, source);
        }
      } /*while (0);*/ while (*++source);
    }
  }

  /* free all memory blocks created */
  rgx_free();
}
