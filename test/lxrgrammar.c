#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "regex.h"
#include "stack.h"

int main () {
  Stack * R = stack_new (0), * A = stack_new (0);
  if (lxr_grammar ( "../languages/basic/lexer.txt", R, A))
    errors (); /* Flush errors, if any */

  char ** rgx = (char **) R->stack, ** action = (char **) A->stack;
  for(int i=0; i<R->len/sizeof (void *); ++i)
    printf ("\n%s %s", rgx[i], action[i]);

  return 0;
}
