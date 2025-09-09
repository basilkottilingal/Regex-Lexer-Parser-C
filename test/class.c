/*
.. test case for equivalence class.
.. $ make obj/class.tst
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "class.h"
#include "nfa.h"

int main () {
  class_init ();
  int list [50], nl = 0;
  for (int i = 'a'; i<= 'z'; ++i)
    list[nl++] = i;

  class_refine (list, nl);                                   /* a-z */
  class_refine (list, 6);                                    /* a-f */

  class_refine (list, 6);        /* a-f */    // should have no effect

  list[0] = '\0';
  class_char ( '\0' );                 /* '\0' is in it's own class */
  class_char ( '\0' );          /* '\0' */    // should have no effect

  /* Expect C(c) in {0,1,2,3} */

  int * class = NULL, nclass = 0;
  class_get ( &class, &nclass );

  char buff[] = " ";
  printf ("\n Eq class for char classes [\\0][a-z][a-f]");
  printf ("\n [  ");
  for (int i=0; i<256; ++i) {
    buff [0] = (char) i;
    printf ("%s %d  ", i>32 && i<126 ? buff : " ", class[i]);
    if ( (i & (int) 15) == 0)
      printf ("\n    ");
  }
  printf ("\n ]");

  /* Now let's see how character class is automatically created
  .. for a regex/regex list*/
  char mem [ 32 ];
  Stack rgxlist = (Stack) {
    .stack = mem, .len = 0, .nentries = 0, .max = sizeof (mem)
  };
  char * rgx[] = { "www[a-fA-F]", "[_a-zA-Z]" };
  stack_push (&rgxlist, rgx[0]);
  stack_push (&rgxlist, rgx[1]);
  nfa_reset (&rgxlist); /* We use the internal function that should
                    .. be called before nfa building */

  /* Print the eq table again */
  class_get ( &class, &nclass );
  printf ("\n Eq class for rgx groups \"%s\" and \"%s\"",
    rgx[0], rgx[1] );
  printf ("\n [  ");
  for (int i=0; i<256; ++i) {
    buff [0] = (char) i;
    printf ("%s %d  ", i>32 && i<126 ? buff : " ", class[i]);
    if ( (i & (int) 15) == 0)
      printf ("\n    ");
  }
  printf ("\n ]");

  return 0;
}
