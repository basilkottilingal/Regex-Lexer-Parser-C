#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "regex.h"
#include "allocator.h"

typedef struct Error {
  struct Error * next;
  char * msg;
} Error;

static Error * list = NULL;

void error ( const char * err, ... ) {
  char msg [256];
  va_list args;      
  va_start (args, err);
  vsnprintf (msg, sizeof (msg), err, args);  
  va_end(args);      

  Error * e =  allocate (sizeof (Error));
  e->msg = allocate_str (msg);
  e->next = list;
}

void errors ( ) {
  Error * e = list, * next;
  while (e) {
    next = e->next;
    fprintf (stderr, "\n  %s", e->msg);
    deallocate (e->msg, strlen (e->msg) + 1);
    deallocate (e, sizeof (Error));
    e = next;
  }
  list = NULL;
  fflush (stderr);
}
