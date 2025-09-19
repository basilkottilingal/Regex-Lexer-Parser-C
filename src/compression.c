#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "allocator.h"
#include "compression.h"

/* ...................................................................
.. ...................................................................
.. ........  Algorithms related to table compression .................
.. ...................................................................
.. .................................................................*/

static int nclass;
static int nstates;

/*
.. The transition mapping δ (q, c) is a matrix of size m⋅n where
.. m = |P(Q)|, and n = |P(Σ)| are the size of minimized dfa set and
.. equivalence class. Expecting demanding scenarios, like a C lexer
.. where m ~ 512 and n ~ 64 (just rounded off. Exact numbers depends
.. on which ANSI C version you are using, etc ), you might need a 2D
.. table size of few MBs and traversing through such a large table
.. for every byte input is a bad idea. In scenarios like a C lexer,
.. where this table is very sparse (δ (q, c) gives a "dead" state),
.. you have a scope of compacting this 2D table into 3 linear
.. tables "check[]", "next[]" and "base[]" as mentioned in many
.. literature and also in codes like flex.
..
.. Along with equivalence class table "class[]" and token info table
.. "accept[]" which stores the most preferred token number for an
.. accepting state, and 0 (DEAD) for a non-accpeting state.
.. |check| = |next| = k, where you minimize k.
.. |accept| = |base| = m
..
.. Transition looks like
..   s <- next [base [s] + class [c]], if check[s] == s
..
.. Size of check[], k, is guaranteed to be in the range [n, m⋅n] but
.. finding the minimal solution is an NP-hard problem and thus rely
.. on greedy heauristic algorithm.
*/

static int compare ( const void * a, const void * b ) {
  #define CMP(p,q) cmp = ((int) (p > q) - (int) (p < q));            \
    if (cmp) return cmp
  Row * s = *((Row **)a), * r = *((Row **)b);
  int cmp;                  /* sort by                              */
  CMP (r->n, s->n);         /* number of transitions (decreasing)   */
  CMP (s->span, r->span);   /* span = end-start  (increasing)       */
  CMP (s->start, r->start); /* first transition  (lowest preferred) */
  CMP (s->s, r->s);         /* state id (lowest preferred)          */
  return 0;
  #undef CMP
}

static void resize (int ** check, int ** next, int * k, int k0) {
  printf ("\n[%d->%d]resize", *k, (*k) +k0); fflush(stdout);
  #define EMPTY -1
  int sold = (*k) * sizeof (int), s = sold + k0 * sizeof(int);
  *check = reallocate (*check, sold, s);
  *next = reallocate (*next, sold, s);
  memset (& (*check) [*k], EMPTY, s - sold);
  (*k) += k0;
  printf ("d"); fflush(stdout);
}

int rows_compression ( Row ** rows, int *** tables, int ** tsize,
  int m, int n ) 
{

  /*
  .. sort the rows by decreasing number of entries, if it matches,
  .. then by increasing span
  */
  qsort (rows, m, sizeof (Row*), compare);
  Row ** row = rows, * r;
  #if 0
  int i = 0; while ( (r=rows[i++]) ) {
    printf ("\ns%2d n%2d start%2d span%2d {",
      r->s, r->n, r->start, r->span);
    for (int j=0; j<r->n; ++j)
      printf ("%d ", r->stack[j]);
    printf ("}");
  }
  #endif

  int k0 = 4 * n;   /* let's start with k=4n & reallocate if needed */
  k0 = 1 << (64 - __builtin_clzll ((unsigned long long)(k0 - 1)) );
  int k = k0;

  int * check = allocate ( k* sizeof (int)),
    * next = allocate (k * sizeof(int)),
    * base = tables [0][2], * accept = tables [0][3],
    * def  = tables [0][4], * meta   = tables [0][5];

  memset ( check, EMPTY, k * sizeof (int) );

  int offset = 0, s;
  while ( (r = *row++) != NULL ) {
    if (offset + 2*n > k)          /* resize next and check if reqd */
      resize (&check, &next, &k, k0);

    if (r->n <= 1) {
      /*
      .. Place all the states with single character differently.
      .. Look for the empty slots beginning from index 0
      */
      int slot = 0, c, delta;
      if (r->n) do {
        s = r->s;
        c = r->stack [0], delta = r->stack [1];
        while ( check [slot] != EMPTY || slot - c < 0) {
          if (++slot + n > k)
            resize (&check, &next, &k, k0);
        }
        next [slot] = delta;
        check [slot] = s;
        accept [s] = r->token;
        base [s] = slot - c;
        if ( base [s] > offset )
          offset = base [s];
      } while ( (r = *row++) && r->n );

      /*
      .. states with zero transitions. place somewhere
      */

      if (r) do {
        s = r->s;
        base [s] = 0;
        accept [s] = r->token;
      } while ( (r = *row++) );
      break;
    }

    s = r->s;
    
    int * c = r->stack, * delta = r->stack + 1;
    for (int i=0; i < r->n; i++) {
      if ( check [offset + *c] != EMPTY ) {      /* collision. */
        i = -1; offset ++;            /* restart, with a new offset */
        c = r->stack;
      }
      c += 2;
    }

    c = r->stack;
    for (int i=0; i < r->n; i++) {
      next [offset + *c] = *delta;
      check [offset + *c] = s;                        /* insert row */
      c += 2; delta += 2;
    }
    accept [s] = r->token;
    base [s] = offset;
  }

  deallocate (rows, (m+1)*sizeof (Row*));

  tsize [0][0] = tsize [0][1] = offset + n;
  tables [0][0] = check;  tables[0][1] = next;

  return 0;
  #undef EMPTY
}
