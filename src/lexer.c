/*
.. This is the main lexer generator file.
.. Requires a lexical rule file which maps regex to an action.
*/

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
.. Copy the "head" part of the lexer generator
.. from src/source.c
*/
int lexer_head ( FILE * out ) {
  char buf[BUFSIZ];
  size_t n;
  FILE *in = fopen("../src/source.c", "r");
  if (!in || !out)
    return RGXERR;

  while ((n = fread(buf, 1, sizeof buf, in)) > 0) {
    if (fwrite(buf, 1, n, out) != n)
      return RGXERR;
  }

  fclose(in);
  return 0;
}

#define LXR_DEBUG
int lexer_tail (FILE * out, Stack * actions) {
  char buf[BUFSIZ];
  FILE *in = fopen("../src/tokenize.c", "r");
  if (!in || !out)
    return RGXERR;

  /*
  .. Make sure ../src/tokenize doesn't have a line
  .. longer than 79 chars
  */
  while (fgets (buf, 80, in)) {
    int j = 0; char c;
    while ( j < 79 && (c = buf [j++]) != '\0' ) {
      if ( c == '/' && buf [j] == '*' && buf [j+1] == '%' ) {
        j = -1; break;
      }
    }
    if (j == -1) break;
    if (fputs (buf,out) == EOF)
      return RGXERR;
  }

  int nactions = actions->len / sizeof (void *);
  char ** action = (char **) actions->stack;
  for (int i=0; i < nactions; ++i) {
    #ifdef LXR_DEBUG
    fprintf (out,
      "\n      case %d :"
      "\n        printf (\"\\ntoken [%3d] %%s\", lxrstrt);"
      "\n        break;", i+1, i+1);
    #else
    fprintf (out,
      "\n      case %d :"
      "\n    %s\n        break;",  i+1,  action [i]);
    #endif
  }

  size_t n;
  fprintf (out, "\n");
  while ((n = fread(buf, 1, sizeof buf, in)) > 0) {
    if (fwrite(buf, 1, n, out) != n)
      return RGXERR;
  }

  fclose(in);
  return 0;
}

/*
.. This is the main function, that read a lexer grammar and
.. create a new source generator, that contains lxr() function
.. which can be used to tokenize a source file.
*/
int lxr_generate (FILE * in, FILE * out) {

  if (in == NULL || out == NULL) {
    error ("lxr : in/out file missing");
    return RGXERR;
  }

  /*
  .. Copies the whole file "src/source.c" at the top of the
  .. lexer generator file
  */
  if (lexer_head (out)) {
    error ("lxr : failed writing lexer head");
    return RGXERR;
  }

  Stack * r = stack_new (64 * sizeof (void *)),
    * a = stack_new (64 * sizeof (void *));
  if (lxr_grammar (in, r, a)) {
    error ("lxr generator : reading grammar failed");
    return RGXERR;
  }

  DState * dfa = NULL;
  char ** rgx = (char **) r->stack;
  int nrgx = r->len/sizeof (void *);
  if (rgx_lexer_dfa (rgx, nrgx, &dfa) < 0) {
    error ("failed to create a minimal dfa");
    return RGXERR;
  }

  /*
  .. print all the tables used by lexer function
  */
  int ** tables, * len;
  if (dfa_tables (&tables, &len) < 0) {
    error ("Table size Out of memory limit");
    return RGXOOM;
  }

  /*
  .. fixme : (a) Optimize, (b) give option to customize datatype,
  .. (c) Make sure, lexer can run on any system (atleast linux),
  .. (d) boundary assertion still not available. (e) Can you
  .. add boundary assertion BOL, EOL, EOF as alphabet/transition
  .. input outside [0x00, 0xFF].
  */

  char * names [] = {
    "check", "next", "base", "accept", "def", "meta", "class"
  };

  char * type [] = {
    "short", "short", "short", "short", "short",
    "unsigned char", "unsigned char"
  };

  /*
  .. write all tables, before main lexer function
  */
  for (int i=0; i<7; ++i) {
    int * arr = tables [i], l = len [i];
    if (!arr) continue;
    fprintf ( out, "\n\nstatic %s %s [%d] = {\n",
      type [i], names[i], l );
    for (int j=0; j<l; ++j) {
      fprintf ( out, " %4d%s", arr[j], j == l-1 ? "" : ",");
      if (j%10 == 0)  fprintf (out, "\n");
      if (j%100 == 0) fprintf (out, "\n");
    }
    fprintf ( out, "\n};" );
  }

  /*
  .. write the main lexer function which executes appropriate
  .. action corresponding to accepted (longest &/ most preferred)
  .. pattern
  */
  if (lexer_tail (out, a)) {
    error ("lxr : failed writing lexer tail");
    return RGXERR;
  }

  #ifdef LXR_DEBUG
  fprintf (out,
    "\nint main () {\n  lxr_lex ();\n  return 0;\n}" );
  #endif

  return 0;
}
