/*
.. test case for nfa.
.. $ make test-nfa.s && ./test-nfa.s
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"

int main () {
  char * rgxlist[] = { "aa|b", "a(a|b)", "bc*", "bc+", "1?", "(a|b)+0" };
  const char txt[] = "aaccbqaabbaba01bcccd";
  char buff[60];

  for (int i=0; i< sizeof (rgxlist) / sizeof (rgxlist[0]); ++i) {

    char * rgx = rgxlist[i];

    DState * dfa[2] =  { NULL } ;
    int status = rgx_dfa ( rgx, dfa );
    if (status < 0) { printf ("_Error"); break; }
    printf ("\n Looking for rgx pattern Regex %s", rgx);
    const char * source = txt;
    do {
      int m = rgx_dfa_match (dfa [status > 0], source);
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
