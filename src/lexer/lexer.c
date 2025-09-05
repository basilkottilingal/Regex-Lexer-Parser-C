#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "stack.h"
#include "allocator.h"
#include "regex.h"

/*
.. Naive way to look for the end of a regex pattern
.. which ends with a white space. The regex whill be stored
.. in the buff
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
    else if ( (c == ' ' || c == '\t') && !charclass) {
      buff[j] = '\0';
      return 0;
    }
    else if (c == '\n')
      break;
  }
  
  if (j == 0)
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
  return ( c == EOF ) ? 1 : 0;
}

int lxr_grammar ( const char * file, Stack * rgxs, Stack * actions ) {

  FILE * fp = fopen (file, "r");
  if(!fp) {
    error ("lxr grammar : cannot open file: %s", file);
    return RGXERR;
  }
  stack_reset (rgxs);
  stack_reset (actions);

  int line = 0, status;
  char rgx [256], action [4096];

  for (;;) {
    status = read_rgx (fp, rgx, sizeof (rgx)); 
    if ( status == RGXERR ) {
      error ("lxr grammar : reading rgx failed."
        "File %s, Line %d", file, line);
      fclose (fp);
      return RGXERR;
    }

    if ( status == EOF )
      break;

    stack_push ( rgxs, allocate_str (rgx) );

    status = read_action (fp, action, sizeof (action), &line);
    if (status == RGXERR ) {
      error ("lxr grammar : reading action failed."
        "File %s, Line %d", file, line);
      fclose (fp);
      return RGXERR;
    }
    stack_push ( actions, allocate_str (action) );
    if (status == 1)
      break;
  }

  fclose (fp);

  if (rgxs->len == 0) {
    error ("lxr grammar : Cannot find any rgx-action pair."
        "File %s, Line %d", file, line);
    return RGXERR;
  }

  return 0;

}
