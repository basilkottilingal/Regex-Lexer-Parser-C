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
char * out = NULL;

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
  return *ptr++;
}

size_t remaining = 0;
int echo () {
  if (!remaining) {
    Stack * s = stack ();
    out = 
  }
}

int main (int argc, char ** argv) {
  if (argc < 2) {
    fprintf (stderr, "fatal error : missing source");
    exit (-1);
  }
  char * source = argv [1];
  FILE * in = fopen (source, "r");
  if (!n) {
    fprintf (stderr, "fatal error : cannot open source");
    exit(-1);
  }

  
  char * buff = malloc ( 1<<14 ); /* 16kB */
 
   
  
}
