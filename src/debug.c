#include <stdio.h>
#include <limits.h>
#include <assert.h>

#include "regex.h"

int rgx_rpn_print ( int * rpn ) {

  #define  PUSH(_c_)  (buff[nbuff++] = _c_)
  #define  POP(_c_)   (nbuff ? buff[--nbuff] : RGXERR)

  int nbuff = 0;
  int buff [RGXSIZE];
  int inode = 512, e1, e2, op, c;
  while ( ( op = *rpn++ ) >= 0 ) {
    if (ISRGXOP (op)) {

      if ( (c = (0xFF&op)) == '^' || c == '$' || c == '[' ||
          c == '<' ) {
        printf ( "\n\t\033[1;32m[  ] =      %c", (char) c);
        continue; 
      }

      printf ( "\n\t\033[1;32m[%2d] =", inode - 512);
      switch (op & 0xFF) {

        case 'd' : case 's' : case 'S' :
        case 'w' : case 'D' : case 'W' :
        case '^' :
          assert (PUSH(inode) >= 0); inode++;
          break;

        case ',' : case ';' : case '-' : case '|' :
          e2 = POP(stack); e1 = POP(stack);
          assert(e1>=0 && e2>=0);
          if (e1>=512)
            printf (" \033[1;31m[%2d]", e1-512);
          else
            printf (" \033[1;31m  %c ", e1);
          printf (" \033[1;32m%c", (char) op);

          if (e2>=512)
            printf (" \033[1;31m[%2d]", e2-512);
          else
            printf (" \033[1;31m  %c ", e2);
          assert (PUSH(inode) >= 0); inode++;
          break;

        case 'q' :
          for (int i=0; i<4; ++i)
            assert (rpn[i] >= 0);
          e1 = POP(stack);
          assert(e1>=0);
          if (e1>=512)
            printf (" \033[1;31m[%2d]", e1-512);
          else
            printf (" \033[1;31m  %c ", e1);
          printf (" \033[1;32m{%d, %d}", rpn[1], rpn [2]);
          rpn += 4;
          assert (PUSH(inode) >= 0); inode++;
          break;
        case '*' : case ']' : case '+' :
        case '?' : case '>' :
          e1 = POP(stack);
          assert(e1>=0);
          if (e1>=512)
            printf (" \033[1;31m[%2d]", e1-512);
          else
            printf (" \033[1;31m  %c ", e1);
          printf (" \033[1;32m%c", (char) op);
          assert (PUSH(inode) >= 0); inode++;
          break;

        default:
      }
    }
    else {
      assert (PUSH(op) >= 0);
    }
  }

  printf("\033[0m");
  fflush(stdout);
  return 0;

  #undef  PUSH
  #undef  POP
}
