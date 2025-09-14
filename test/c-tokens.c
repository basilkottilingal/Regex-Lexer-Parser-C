#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "regex.h"
#include "stack.h"

int main () {

  FILE * fp = fopen ("../languages/c/c.lex", "r");
  //FILE * fp = fopen ("../languages/c/error.lex", "r");
  if(!fp) {
    error ("lxr grammar : cannot open file");
    return RGXERR;
  }

  if (lxr_generate (fp, stdout)){
    errors();
  }

  fclose (fp);

  return 0;
}
