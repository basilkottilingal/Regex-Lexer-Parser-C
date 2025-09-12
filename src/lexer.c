#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "stack.h"
#include "allocator.h"
#include "regex.h"
#include "lexer.h"
#include "class.h"

/*
.. Naive way to look for the regex pattern at the beginning of the
.. line. The regex will be stored in "buff".
*/
static
int read_rgx (FILE * fp, char * buff, size_t lim) {
  int c, j = 0, escape = 0, charclass = 0;
  while ( (c = fgetc(fp)) != EOF ) {
    buff[j++] = (char) c;
    if (j == lim) {
      error ( "lxr grammar: rgx buff limit" );
      return RGXERR;
    }
    if (escape) {
      escape = 0;
      continue;
    }
    if (c == '\\') {
      escape = 1;
      continue;
    }
    if (c == '[')
      charclass = 1;
    else if (c == ']' && charclass )
      charclass = 0;
    /* White space are taken as end of regex */
    else if ( (c == ' ' || c == '\t') && !charclass) {
      buff[--j] = '\0';
      if ( j == 0 ) {
        while ( (c = fgetc(fp)) != EOF && c != '\n' ) {
          if ( !(c == ' '|| c == '\t') )  {
  error ("lxr grammar: rgx should be at the beginning of the line");
  return RGXERR;
          }
        }
        #define EMPTY 10
        return EMPTY;         /* Empty line, with just white spaces */
      }
      return 0;
    }
    else if (c == '\n') {
      if ( j == 1 )
        return EMPTY;                                 /* Empty line */
      break;
    }
  }

  if ( j == 0 )
    return EOF;

  error ("lxr grammar : Incomplete rgx or missing action");
  return RGXERR;
}

/*
.. Naive way to read action. The action should be put inside a '{' '}'
.. block and the starting '{' should be placed in the same line of
.. the regex pattern
*/
static
int read_action (FILE * fp, char * buff, size_t lim, int * line) {
  /* fixme : remove the limit. use realloc() to handle large buffer */

  int c, j = 0, depth = 0, quote = 0, escape = 0;
  /*
  .. Consume white spaces before encountering '{' i.e start of action.
  .. Warning : Action need to be in the same line as regex.
  */
  while ( (c = fgetc(fp)) != EOF ) {
    if ( c == '{' ) {
      depth++;
      buff [j++] = '{';
      break;
    }
    if ( !(c == ' ' || c == '\t' ) ) {
      error ("lxr grammar : Unexpected character"
        "between regex & action");
      return RGXERR;
    }
  }

  /*
  .. Doesn't validate C grammar that falls in action block { ... }.
  .. The whole action block will be stored inside 'buff' unless
  .. you run out of limit.
  */
  while ( (c = fgetc(fp)) != EOF ) {
    buff[j++] = (char) c;
    if (j == lim) {
      error ( "lxr grammar: action buff limit" );
      return RGXERR;
    }

    if (c == '\n')
      (*line)++;

    if (quote) {
      if (escape)
        escape = 0;
      else if (c == '\\')
        escape = 1;
      else if (c == '"') {
        quote  = 0;
        continue;
      }
    }
    else {
      if (c == '"') {
        quote = 1;
        continue;
      }
      if (c == '{')
        depth++;
      else if (c == '}') {
        depth--;
        if (!depth)
          break;
      }
    }
  }
  buff[j] = '\0';

  if (depth) {
    error ("lxr grammar : incomplete action. Missing '}'");
    return RGXERR;
  }

  while ( (c = fgetc(fp)) != EOF ) {
    if ( c == '\n') {
      (*line)++;
      break;
    }
    if ( ! (c == ' ' || c == '\t' ) ) {
      error ("lxr grammar : unexpected character after action");
      return RGXERR;
    }
  }
  return 0;
}

/*
.. Read regex pattern, action pair from "in"
*/
static
int lxr_grammar ( FILE * in, Stack * rgxs, Stack * actions ) {

  stack_reset (rgxs);
  stack_reset (actions);

  int line = 1, status;
  char rgx [RGXSIZE], action [4096];

  for (;;) {
    /* Read rgx pattern */
    status = read_rgx (in, rgx, sizeof (rgx));
    if (!status)
      stack_push ( rgxs, allocate_str (rgx) );
    else if ( status == EOF )
      break;
    else if ( status == EMPTY ) {
      line++;
      continue;
    }
    else if ( status == RGXERR ) {
      error ("lxr grammar : reading rgx failed."
        "Line %d", line);
      fclose (in);
      return RGXERR;
    }
    #undef EMPTY

    /* Read the action for the above regex pattern */
    status = read_action (in, action, sizeof (action), &line);
    if (status == RGXERR ) {
      error ("lxr grammar : reading action failed."
        "Line %d", line);
      fclose (in);
      return RGXERR;
    }
    stack_push ( actions, allocate_str (action) );
  }

  if (rgxs->len == 0) {
    error ("lxr grammar : Cannot find any rgx-action pair."
        "Line %d", line);
    return RGXERR;
  }
  return 0;
}

/*
.. This is the main function, that read a lexer grammar and
.. create a new source generator, that contains lxr() function
.. which can be used to tokenize a source file.
*/
int lxr_generate (FILE * in, FILE * out) {

  Stack * r = stack_new (64 * sizeof (void *)),
    * a = stack_new (64 * sizeof (void *));
  if (lxr_grammar (in, r, a)) {
    error ("lxr generator : reading grammar failed");
    return RGXERR;
  }

  DState * dfa = NULL;
  char ** rgx = (char **) r->stack;
  int nrgx = r->len/sizeof (void *);
  if (rgx_list_dfa (rgx, nrgx, &dfa) < 0) {
    error ("failed to create a minimal dfa");
    return RGXERR;
  }

  int * class, nclass;
  class_get (&class, &nclass);
  char * names [] = {
    "class", "next", "accept", "base" };
  int * table [] = { 
    class /* fixme : */ };
  int len [] = { 256 };
  
  for (int i=0; i<1/* fixme */; i++) {
    fprintf (out, "\nstatic int lxr_%s [] = {\n", names [i]);
    for (int j=0; j<len[i]; ++j) {
      fprintf (out, "  %3d", table [i][j]);
      if (j%10 == 0) 
        fprintf (out, "\n");
    }
    fprintf (out, "\n};");
  }

  /*
  fprintf (out, "\n  while ( (c = lxr_input) != EOF ) { "
    "\n    if (accept) {"
    "\n      last = lxr_accept [ ]"
    "\n    }");
  */

  int n = a->len / sizeof (void *);
  char ** action = (char **) a->stack;
  for (int i=0; i<n; ++i) {
    fprintf (out, "\n  case %d :"
      "\n    %s"
      "\n    break;",
      i, action [i]);
  }
  fprintf (out, "\n  default : "
    "\n    fprintf (stderr, \"lxr aborted\");"
    "\n    fflush (stderr); "
    "\n    exit (EXIT_FAILURE);" );

  return 0;
}
