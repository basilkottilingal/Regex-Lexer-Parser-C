/*
.. test case for nfa.
.. $ make test-nfa.s && ./test-nfa
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"
#include "nfa.h"
#include "stack.h"

int main () {
  char * rgx[] = { "aa|b", "a(a|b)", "bc*", "bc+", "(a|b)+0", "bc[a-z]+" };
  const char txt[] = "aaccbqaabbaba01bcccd";
  char buff[60];
  printf ("Regex group ");
  int nrgx = sizeof (rgx) / sizeof (rgx[0]);
  for (int i=0; i<nrgx; ++i) {
    printf ("\"%s\" ", rgx[i]);
  }
  DState * dfa = NULL;
  int status = rgx_list_dfa (rgx, nrgx, &dfa);
  if (status < RGXEOE)  
    printf ("failed in creating NFAi for rgx group"); 

  const char * source = txt;
  if (status > 0 ) do {
    int m = rgx_dfa_match (dfa, source);
    if (m < 0) printf ("failed in match check %d", m); 
    //source += m > 1 ? (m-1) 
    if (m > 0) {
      m -= 1; buff[m] = '\0';
      if(m) memcpy (buff, source, m);
      printf ("\nRegex: found in txt \"%s\" (\"%s\")", buff, source);
    }
  } while (/*0*/*++source);

  /* free all memory blocks created */
  rgx_free();
}
