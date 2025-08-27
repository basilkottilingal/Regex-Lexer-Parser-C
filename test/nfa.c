/*
.. test case for nfa.
.. $ make test-nfa.s && ./test-nfa
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"

int main () {
  char * rgx[] = { "aa|b", "a(a|b)", "bc*", "bc+", "1?", "(a|b)+0" };
  const char txt[] = "aaccbqaabbaba01bcccd";
  char buff[60];
  State * nfa = NULL;
  for (int i=0; i< sizeof (rgx) / sizeof (rgx[0]); ++i) {

    int status = rgx_nfa (rgx[i], &nfa);
    if (status < RGXEOE) 
      printf ("failed in creating NFAi for rgx %s: %d", rgx[i], status); 

    const char * source = txt;
    do {
      int m = rgx_nfa_match (nfa, source);
      if (m < 0) printf ("failed in match check %d", m); 
      //source += m > 1 ? (m-1) 
      if (m > 0) {
        m -= 1; buff[m] = '\0';
        if(m) memcpy (buff, source, m);
        printf ("\nRegex %s: found in txt \"%s\" (\"%s\")", rgx[i], buff, source);
      }
    } while (*++source);
  }

  /* free all memory blocks created */
  rgx_free();
}
