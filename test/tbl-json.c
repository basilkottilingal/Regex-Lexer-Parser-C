/*
.. test case for nfa.
.. $ make test-nfa.s && ./test-nfa
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"
#include "nfa.h"

int main () {
  char * rgx[] = { 
    "true", "false", "null", 
    "[ \\t\\v\\n\\f\\r]+", "\\[", "\\]", ":", ",", "\\{", "\\}", 
    "-?(0|[1-9][0-9]*)(\\.[0-9]+)?([eE][+-]?[0-9]+)?", 
    "\"([^\"\\\\]|\\\\[\"\\/bfnrt])*\"" /* fixme : allow unicode */
  };

  int nrgx = sizeof (rgx) / sizeof (rgx[0]);
  printf ("json regex patterns");
  for (int i=0; i<nrgx; ++i)
    printf ("\n%s", rgx[i]);
  DState * dfa = NULL;
  int status = rgx_list_dfa (rgx, nrgx, &dfa);
  if (status < 0) {
    errors ();
    printf ("cannot make DFA. aborting");
    fflush (stdout);
    exit (-1);
  }

  printf ("\njson tables"); fflush (stdout);
  int ** tables, * len;
  dfa_tables (&tables, &len);

  char * names [] = {
    "check", "next", "base", "accept", "class"
  };
  for (int i=0; i<5; ++i) {
    int * arr = tables [i], l = len [i];
    printf ("\n\nint %s [%d] = {\n", names[i], l);
    for (int j=0; j<l; ++j)
      printf ("  %3d%s", arr[j], j%10 ? "" : "\n");
    printf ("\n}");
  }

  /* free all memory blocks created */
  rgx_free();
}
