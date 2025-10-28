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

#define MEMSIZE (1<<16)

Stack * stack () {
  Stack * s = malloc (sizeof (Stack));
  if (!s) {
    fprintf(stderr, "out of dynamic memory : malloc failed");
    exit (-1);
  }
  if ( !(s->buff = malloc (MEMSIZE)) ) {
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
  fprintf (stdout, "%s", buff);
}

void push (int c) {
  if (bptr == lim) {
    size_t size = (lim - buff) + 1,
      older = strt-buff,
      thisline = lim-strt;
    if ( thisline > (size_t) (0.8* size) ) {
      strt = buff = realloc (buff, 2*size);
      if (!buff) {
        fprintf(stderr, "out of dynamic memory : malloc failed");
        exit (-1);
      }
      lim = & buff [2*size-1];
      bptr = & buff [size-1];
    }
    else {
      fwrite (buff, 1, older, stdout);
      memmove (buff, strt, thisline);
      strt = buff;
      bptr = & buff [thisline];
    }
  }
  *bptr++ = (char) (unsigned char) c;
}

void eol () {
  push ('\n');
  strt = bptr;
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

  bptr = buff = malloc (MEMSIZE);
  if (!buff) {
    fprintf(stderr, "out of dynamic memory : malloc failed");
    exit (-1);
  }
  lim = & bptr [MEMSIZE-1];
 
  int c, quote = 0, escape = 0, comment = 0;
  while ( (c=input()) != EOF ) {
    if (c == '\n') {
      quote = escape = comment = 0;
      echo ();  
    }

    if (quote) {
      if (escape)
        escape = 0;
      else if (c == '\\')
        escape = 1;
      else if (c == quote)
        quote = 0;
      echo (c);
      continue;
    }

    switch (c) {
      case '/' :
        comment = 1;
        break;
      case '"'  :
      case '\'' :
        quote = c;
        break;
      case '{' :
        depth++;
        break;
      case '}' :
        depth--;
        break;
      case 
    }
    
  }
  echo (EOF);

  fclose(fp);

  Stack * s = head;
  while (s){
    Stack * next = s->next;
    free (s->buff);
    free (s);
    s = next;
  }
}
