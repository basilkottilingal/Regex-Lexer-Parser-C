#include <stdio.h>
#include <stdlib.h>

#include "../src/source.c"

int main () {
  FILE * fp = fopen ("../src/source.c", "r");

  lxrin_set (fp);

  int c;
  while ( (c = lxr_input ()) != '\0' ) {
    printf ("%c", (char) c);
  };

  lxr_clean ();

  fclose (fp);

  return 0;
}

