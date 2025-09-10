
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"

int main () {
  char * rgx[] = { "N[a-z1-3;]", "[^a-f?]|swww*" };
  int rpn [RGXSIZE];
  for (int i=0; i< sizeof (rgx) / sizeof (rgx[0]); ++i) {
    printf ("\nRpn for rgx \"%s\"", rgx[i]);
    rgx_rpn (rgx[i], rpn);
    rgx_rpn_print (rpn);
  }

  rgx_free();
}
