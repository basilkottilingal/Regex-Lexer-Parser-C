/*
.. Translation phases 2 and 3.
.. (a) Replaces multiline comment "\*".*"*\" with " "
.. (b) Consumes single line comment "\\".*,
.. (c) Replaces "%:" with "#"
.. (d) Replaces "%:%:" with "##"
.. (e) Consumes line splicing [\\][\n][ \t]*$
.. (f) Consumes trailing white spaces [ \t]+$
.. Assumes translation phase 1 is obsolete (and not required).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Stack {
  char * buff;
  struct Stack * next;
} Stack;

Stack * head = NULL;

FILE * fp;

char * eob = NULL;
char * eof = (char *) 1;
char * ptr = NULL;

#define MEMSIZE (1<<14)

Stack * stack () {
  Stack * s = malloc (sizeof (Stack));
  if ( !s || !(s->buff = malloc (MEMSIZE)) ) {
    fprintf(stderr, "out of dynamic memory : malloc failed");
    exit (-1);
  }
  s->next = head;
  head = s;
  return s;
}

int input () {
  if (ptr == eob) {
    if (ptr == eof)
      return EOF;
    Stack * s = stack ();
    size_t n = fread (s->buff, 1, MEMSIZE - 1, fp);
    eob = s->buff + n;
    ptr = s->buff;
    if (n < MEMSIZE - 1) {
      if (!feof (fp)) {
        fprintf (stderr, "fread () failed");
        exit (-1);
      }
      eof = s->buff + n;
      if (!n) return EOF;
    }
  }
  return (int) (unsigned char) *ptr++;
}

char * buff;
char * bptr;
char * strt;
char * lim;

void echo () {
  fwrite (buff, 1, strt-buff, stdout);
  memmove (buff, strt, bptr-strt);
  bptr = & buff [bptr-strt];
  strt = buff;
}

void push (int c) {
  if (bptr == lim) {
    size_t size = (lim - buff) + 1;
    if ( bptr-strt > (size_t) (0.8* size) ) {
      strt = buff = realloc (buff, 2*size);
      if (!buff) {
        fprintf(stderr, "out of dynamic memory : malloc failed");
        exit (-1);
      }
      lim = & buff [2*size-1];
      bptr = & buff [size-1];
    }
    else
      echo ();
  }
  *bptr++ = (char) (unsigned char) c;
}


int main (int argc, char ** argv) {
  if (argc < 2) {
    fprintf (stderr, "fatal error : missing source");
    exit (-1);
  }
  char * source = argv [1];
  fp = fopen (source, "r");
  if (!fp) {
    fprintf (stderr, "fatal error : cannot open source");
    exit(-1);
  }

  strt = bptr = buff = malloc (MEMSIZE);
  if (!buff) {
    fprintf(stderr, "out of dynamic memory : malloc failed");
    exit (-1);
  }
  lim = & bptr [MEMSIZE-1];

  #define pop()   bptr--
 
  int c, quote = 0, escape = 0, comment = 0, splicing,
    percent = 0;
  while ( (c=input()) != EOF ) {
        
    push (c);

    if (c == '\n') {
      pop();
      if (quote) {
        if (escape) {
          escape = 0;
          pop();
          continue;
        }
        quote = 0;      
        push ('\n');  /* may warn. unclosed quote. */
        strt = bptr;
        continue;
      }

      /*
      .. pop trailing white spaces, and splicing "\\\n" if found
      */
      splicing = 0;
      while ( (bptr != strt) ) {
        if ( (c = bptr[-1]) == '\\' ) {
          if ( splicing )
            break;
          splicing = 1;
          pop();
          continue;
        }
        if ( !(c == ' ' || c == '\t' ) )
          break;
        pop();
      }
      if (!splicing)
        push ('\n');
      strt = bptr;
      
      continue;
    }

    if (quote) {
      if (escape)
        escape = 0;
      else if (c == '\\')
        escape = 1;
      else if (c == quote)
        quote = 0;
      continue;
    }

    if (comment) {
      comment  = 0;
      if ( c == '/' ) {
        pop (); pop ();
        /* consuming single line comment */
        while ( (c = input()) != EOF ) {
          if ( c == '\n' )
            break;
        }
        /* consume white space before "\\" */
        while ( (bptr != strt) ) {
          if ( ! ((c = bptr[-1]) == '\\' || c == '\t' ) )
            break;
          pop();
        }
        continue;
        push ('\n');
      }
      else if ( c == '*') {
        pop (); pop ();
        /* consuming multi line comment */
        while ( (c = input()) != EOF ) {
          if ( c != '*' ) continue;
          while ( (c = input()) == '*' ) {
          }
          if ( c == '/' ) break;
        }
        push (' ');
        continue;
      }
    }

    if (percent) {
      /* converting digraph "%:" to "#" */
      percent = 0;
      if (c == ':') {
        pop (); pop ();
        push ('#');
        continue;
      }
    }

    switch (c) {
      case '/' :
        comment = 1;
        break;
      case '"'  :
      case '\'' :
        quote = c;
        break;
      case '%' :
        percent = 1;
        break;
      default :
        break;
    }
    
  }
  echo ();

  fclose(fp);

  Stack * s = head;
  while (s){
    Stack * next = s->next;
    free (s->buff);
    free (s);
    s = next;
  }
  free (buff);
}
