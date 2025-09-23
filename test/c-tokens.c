#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "regex.h"
#include "stack.h"

int main () {

  FILE * fp = fopen ("../languages/c/c99.lex", "r"),
    * out = fopen ("../languages/c/c99.c", "w");

  /* Create the lexer header file to "out"*/
  if ( lxr_generate (fp, out) ) {
    errors();
    exit (-1);
  }

  fclose (fp);

  return 0;
}
