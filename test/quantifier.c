#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"
#include "nfa.h"

int main () {
  char * rgx[] = { 
    "a{1,2}", "www{1,1}", "lmn{0,1}", 
    "[50]{4,5}", "bc((a|b)c?[0-9]{2}){,5}[wW]",
    "a{1,}"
  };

  int rpn [RGXSIZE];
  int nrgx = sizeof (rgx) / sizeof (rgx[0]);
  nfa_reset (rgx, nrgx);

  State * nfa = NULL;
  for (int i=0; i< sizeof (rgx) / sizeof (rgx[0]); ++i) {
      
    printf ("\n\nrgx %s ", rgx[i]);
    rgx_rpn (rgx[i], rpn);
    rgx_rpn_print (rpn);

    int status = rgx_nfa (rgx[i], &nfa, 0);
    if (status < RGXEOE) { 
      printf ("failed in creating NFAi for rgx %s: %d", rgx[i], status); 
    }
    #if 0
    const char txt[] = "aaccbqaabbaba01bcccd";
    char buff[60];
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
    #endif
  }

  /* free all memory blocks created */
  rgx_free();
}
