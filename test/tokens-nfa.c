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
  char * rgx[] = { "aa|b", "a(a|b)", "bc*", "bc+", "1?", "(a|b)+0" };
  const char txt[] = "aaccbqaabbaba01bcccd";
  char buff[60];
  Stack * stk = stack_new (0);
  printf ("Regex group ");
  for (int i=0; i< sizeof (rgx) / sizeof (rgx[0]); ++i) {
    printf ("\"%s\" ", rgx[i]);
    stack_push (stk, rgx[i]);
  }
  DState * dfa = NULL;
  int status = rgx_list_dfa (stk, &dfa);
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
