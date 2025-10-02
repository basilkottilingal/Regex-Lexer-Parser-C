/*
.. Read a lex file. Follows the flex's ".lex" format. You can refer to
.. the flex documentation on the input file format here
.. https://westes.github.io/flex/manual/Format.html#Format
.. Input file need to follow the format
..
.. definitions
.. %%
.. rules
.. %%
.. user code
..
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"
#include "allocator.h"
#include "lexer.h"

/*
.. Flex macro that holds a regex pattern. Macro should be in the form
.. [_a-zA-Z][_a-zA-Z0-9]*. You can recurse macro, i.e a macro can use
.. a previous macro, in it's definition. Macros are defined in the 
.. definition section. Examples of macros are given below
.. 
.. ALPH   [_a-zA-Z]
.. DIGITS [0-9]
.. ID     {ALPHA}({ALPH}|{DIGITS})*
*/

typedef struct Macro {
  char * key, * pattern;
  struct Macro * next;
} Macro;

/*
.. A code snippet is defined in the definitions section in either of
.. the two forms as given below
..
.. %top {
..   #include <stdio.h>
..   #include "func.h"
.. }
.. 
..%{
..   #include <stdio.h>
..   #include "func.h"
..%}
*/

typedef struct Snippet {
  char * code;
  struct Snippet * next, * prev;
  int flag;
} Snippet;


/*
.. patterns are defined exclusively in the rules section, that falls
.. in between two %% separators. The pattern should be followed by the
.. corresponding action. Pattern is defined in the POSIX extended
.. regular expressions (ERE) with few exceptions
.. (a) strings are allowed in quotes
.. (b) macros inside {}
.. (c) . (dot or "all-character" class) excludes '\n'
.. (d) anchors '^' and '$' are allowed only at the beginning/end of the
..     tokens respectively 
..
.. %%
.. ID       { return IDENTIFIER; }
.. %%
*/
typedef struct Pattern {
  char * pattern, * action;
  int flag;
} Pattern;
