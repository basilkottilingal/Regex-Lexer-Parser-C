#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "stack.h"
#include "allocator.h"

int actions ( const char * file, Stack * rgxs, Stack * actions ) {
  #define ERR(e) fprintf (stderr, "%s"); fflush (stderr);\
    if(fp) fclose(fp); return RGXERR

  FILE * fp = fopen (file, "r");
  if(!fp) { ERR ("lxr grammar : cannot open file"); }

  stack_reset (rgxs); stack_reset (actions);
  
  char rgx [128], action [4096];

  int c = '\0', i, j;
  for ( ;c != EOF; ) {
    i = j = 0;

    while ( (c = getchar()) != EOF && c != '{' ) {
      if ( i == sizeof (rgx) - 1 ) {
        ERR ("lxr grammar : Out of rgx buffer limit.");
      }
      rgx [i++] = (char) c;
    }
    rgx [i] = '\0';

    if ( i == 0 ) {
      ERR ("lxr grammar : cannot read regex ");
    }

    action [j++] = '{';
    while ( (c = getchar()) != EOF ) {
      if ( j == sizeof (action) - 1) {
        ERR ("lxr grammar : Out of action buffer limit.");
      }
      action [j++] = (char) c;
      if (c == '}')  {
        while ( (c = getchar()) != EOF && c != '\n' ) { }
        break;
      }
    }
    action [j] = '\0';
    if ( action [j-1] != '}') {
      ERR ("lxr grammar : cannot read action");
    }

    stack_push ( rgxs, allocate_str (rgx) );
    stack_push ( actions, allocate_str (action) );
    
  }
  
  if (rgxs->len == 0) {
    ERR ("lxr grammar : Cannot find any rgx-action pair. ");
  }

  fclose (fp);
  return (0);

  #undef ERR
}
