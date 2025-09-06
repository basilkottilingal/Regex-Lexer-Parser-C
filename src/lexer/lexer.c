#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "stack.h"
#include "allocator.h"
#include "regex.h"

/*
.. Naive way to look for the regex pattern at the beginning of the
.. line. The regex whill be stored in "buff".
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

int lxr_grammar ( const char * file, Stack * rgxs, Stack * actions ) {

  FILE * fp = fopen (file, "r");
  if(!fp) {
    error ("lxr grammar : cannot open file: %s", file);
    return RGXERR;
  }
  stack_reset (rgxs);
  stack_reset (actions);

  int line = 1, status;
  char rgx [256], action [4096];

  for (;;) {
    /* Read rgx pattern */
    status = read_rgx (fp, rgx, sizeof (rgx));
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
        "File %s, Line %d", file, line);
      fclose (fp);
      return RGXERR;
    }
    #undef EMPTY

    /* Read the action for the above regex pattern */
    status = read_action (fp, action, sizeof (action), &line);
    if (status == RGXERR ) {
      error ("lxr grammar : reading action failed."
        "File %s, Line %d", file, line);
      fclose (fp);
      return RGXERR;
    }
    stack_push ( actions, allocate_str (action) );
  }

  fclose (fp);
  if (rgxs->len == 0) {
    error ("lxr grammar : Cannot find any rgx-action pair."
        "File %s, Line %d", file, line);
    return RGXERR;
  }
  return 0;
}

/*
.. This is the main function, that read a lexer grammar and
.. create a new source generator, that contains lxr() function
.. which can be used to tokenize a source file.
*/
int lxr_generator (const char * grammar, const char * output ){

  FILE * fp = output ? fopen (output, "w") : stdout;
  if ( !grammar || !fp ) {
    error ("lxr generator : aborted");
    return RGXERR;
  }

  Stack * r = stack_new (0), * a = stack_new (0);
  if (lxr_grammar (grammar, r, a)) {
    error ("lxr generator : reading grammar failed");
    return RGXERR;
  }

  DState * dfa = NULL;
  if (rgx_list_dfa (r, &dfa) < 0) {
    error ("failed to create a minimal dfa");
    return RGXERR;
  }

  fprintf (stdout, "\n static int lxr_table [] ");
  fprintf (stdout, "\n static int lxr_next [] ");
  fprintf (stdout, "\n static int lxr_accept [] ");

  fprintf (stdout, "\n  while ( (c = lxr_input) != EOF ) { "
    "\n    if (accept) {"
    "\n      last = lxr_accept [ ]"
    "\n    }");

  int n = a->len / sizeof (void *);
  char ** action = (char **) a->stack;
  for (int i=0; i<n; ++i) {
    fprintf (stdout, "\n  case %d :"
      "\n    %s"
      "\n    break;",
      i, action [i]);
  }
  fprintf (stdout, "\n  default : "
    "\n    fprintf (stderr, \"lxr aborted\");"
    "\n    fflush (stderr); "
    "\n    exit (EXIT_FAILURE);" );

  if (output) fclose (fp);

  return 0;
}
