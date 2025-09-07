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

int main () {
  class_init ();
  int list [50], nl = 0;
  for (int i = 'a'; i<= 'z'; ++i)
    list[nl++] = i;

  class_refine (list, nl);  /* a-z */
  class_refine (list, 6);  /* a-f */
  for (int i=0; i<100; i++)
    class_refine (list, 6);  /* a-f */ // should have no effect


  int * class = NULL, nclass = 0;
  class_get ( &class, &nclass );

  char buff[16]; buff[1] = '\0';
  for (int i=0; i<256; ++i) {
    buff [0] = (char) i;
    printf ("%s %d  ", i>32 && i<126 ? buff : " ", class[i]);
    if ( (i & (int) 15) == 0)
      printf ("\n");
  }

  return 0;
}
