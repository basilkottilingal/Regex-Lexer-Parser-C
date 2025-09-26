#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "allocator.h"
#include "compression.h"
#include "regex.h"

/* ...................................................................
.. ...................................................................
.. ........  Algorithms related to table compression .................
.. ...................................................................
.. .................................................................*/
#define PARENT_THRESHOLD     0.7
#define CHILD_THRESHOLD      0.9
#define SMALL_THRESHOLD      5
#define EMPTY                -1

/*
.. A row is a cache corresponding to a state 's', which stores a 
.. cache of (c, δ (s,c)) where the transition δ (s,c) is not a DEAD
.. state.
..    cache(s) := { (c, δ (s,c)) | δ (s,c) ≠ DEAD }
*/

/*
.. Variables used for local use only
*/
typedef struct Delta {
  int c, delta;
} Delta;
static int * check;                                  /* check array */
static int * next;                                    /* next array */
static int * base;                                    /* base array */
static int * def;
static int nclass;                          /* number of eq classes */
static int nstates;                             /* number of states */
static int limit;                 /* allocated size of check & next */


/*
.. For a candidate state 'ps' which is already added to check[], see
.. the number of exact transitions (c, δ (s,c)), not found in the
.. candidate's
*/
static inline
int row_candidate ( Row * r, int ps, Delta residual [] ) {
  int n = 0, i = r->n, c = nclass-1, 
    * chk = & check [base [ps]],
    * nxt = & next [base [ps]];
  Delta * a = (Delta *) r->stack;
  while ( i-- >= 0) {
    int min = i >= 0 ? a[i].c : -1;
    while (c > min) {
      int _s = ps;
      while (_s != EMPTY && check [base [_s]+c] != _s){
        _s =def [_s]; 
      }
      if ( _s != EMPTY && next [base [_s]+c] != EMPTY )
        residual [n++] = (Delta) {c, EMPTY};
      c--;
    }
    if (i < 0) break;
    if ( chk [c] != ps || nxt [c] != a[i].delta )
      residual [n++] = a [i];
    c--;
  }
  return n;                                    /* size of residual. */
}

#if 0
void debug (Row * r, int ps, Delta residual []) {

  if(ps == EMPTY) return;
  int nk = row_candidate (r, ps, residual);
  printf("\n placing row %d def %d {\n", r->s, ps);
  for (int i=0; i<nk;++i)
    printf("(%d,%d) ", residual[i].c, residual[i].delta);
  printf ("}");
  //return n;                                    /* size of residual. */
}
#endif

/*
.. Look for a slot to insert a cache of (c, δ (s,c))
*/
static
int find_slot ( Delta residual [], int nr ) {
  for (int offset=0; offset<=limit - nclass; ++offset) {
    int n = nr;
    while (n--)
      if ( check [offset + residual [n].c] != EMPTY )
        break; 
    if (n >= 0) continue;
    return offset;
  }
  return EMPTY;
}

/*
.. Insert a row to check[] ( and corresponndingly to next []),
.. given a parent candidate 'ps' (which may be empty)
*/
static
int row_insert ( Row * r, int ps, Delta residual [] ) {
  Delta * res = (ps != EMPTY) ? residual : (Delta *) r->stack;
  int s = r->s, nr = (ps != EMPTY) ? 
    row_candidate (r, ps, residual) : r->n;
  if (!nr)
    return (base [s] = 0);
  int offset = find_slot (res, nr);
  if ( offset == EMPTY ) return RGXERR;
  int n = nr;
  while (n--) {
    check [offset + res [n].c] = s;
    next [offset + res [n].c] = res [n].delta;
  }
  return (base[s] = offset);
}

/*
.. How similar are two rows
*/
static inline
int row_similarity ( Row * r, Row * c ) {
  int n = 0;
  Delta * a = (Delta *) r->stack, 
    * b = (Delta *) c->stack;
  int i = r->n-1, j = c->n-1;
  while ( i>=0 && j>=0 ) {
    if (  a[i].c == b[j].c ) {
      if ( a[i].delta == b[j].delta ) n++;
      i--, j--;
    }
    else if ( a[i].c > b[j].c ) i--;
    else j--;
  }
  return n;
}

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
  CMP (s->hash, r->hash);   /* Compare signature                    */
  CMP (s->s, r->s);         /* state id (lowest preferred)          */
  return 0;
  #undef CMP
}

static int resize (int k0) {
  int sold = limit * sizeof (int), s = sold + k0 * sizeof(int);
  if (s > PAGE_SIZE) return RGXOOM;
  check = reallocate (check, sold, s);
  next = reallocate (next, sold, s);
  memset (& check [limit], EMPTY, s - sold);
  limit += k0;
  return 0;
}


#if 0
void row_print ( Row ** rows) {
  int i = 0; Row * r; while ( (r=rows[i++]) ) {
    printf ("\ns%2d n%2d hash%2u token%2d {",
      r->s, r->n, r->hash, r->token);
    for (int j=0; j<r->n; ++j)
      printf ("%s%3d[%3d] ",
        j%16 ? "" :"\n\t",
        r->stack[2*j], r->stack[2*j+1]);
    printf ("}");
  }
}
#endif

int rows_compression ( Row ** rows, int *** tables, 
  int ** tsize, int m, int n )
{

  nstates = m, nclass = n;
  Delta residual [256];

  /*
  .. sort the rows by decreasing number of entries, if it matches,
  .. then by increasing span
  */
  qsort (rows, m, sizeof (Row*), compare);

  int k0 = 4 * n;   /* let's start with k=4n & reallocate if needed */
  k0 = 1 << (64 - __builtin_clzll ((unsigned long long)(k0 - 1)) );
  limit = k0;

  check = allocate (limit* sizeof (int));
  next = allocate (limit * sizeof(int));
  base = tables [0][2];
  def  = tables [0][4];
  int * accept = tables [0][3];
  #if 0
  int * meta   = tables [0][5];  /* We don't use meta class for now */
  #endif

  memset ( check, EMPTY, limit * sizeof (int) );

  int offset = 0;   Row * r;
  for (int irow=0; (r = rows [irow]) != NULL; ++irow ) {

    if (offset + 2*n > limit)  /* resize next[] and check[] if reqd */
      if (resize (k0)) {
        error ("Table compression: Out of table size limit %d",
          (int) PAGE_SIZE / sizeof (int));
        return RGXOOM;
      }

    int s = r->s, jrow = irow - 1, nrows = 0, min = INT_MAX,
      best = EMPTY, queue [2];
    /*
    .. We look among the rows that are already added, to see if 
    .. it is a good candidate to be taken as the def[this state].
    .. The best candidate is chosen by minimum of |residual|,
    .. where residual is the set of transitions (c, delta) which are
    .. not found in the cache of candidate.
    */
    while (jrow >= 0 && nrows++ < 8) {
      queue [0] = rows [jrow]->s, queue [1] = def [queue [0]];
      for (int iq =0; iq < 2 && queue [iq] != EMPTY; ++iq) {
        int nres = row_candidate ( r, queue [iq], residual );
        if (nres < min) { min = nres; best = queue [iq]; }
      }
      jrow --;
    }

    if ( best != EMPTY && min > r->n )
      best = EMPTY;

    if ( best != EMPTY &&
      ( rows [irow+1] != NULL && r->n > SMALL_THRESHOLD ) &&
      ( min > (int) ((1.0 - PARENT_THRESHOLD) * r->n) ) &&
      ( row_similarity (r, rows[irow+1]) > 
        (int) (CHILD_THRESHOLD* r->n)) ) 
    {
      /*
      .. After looking for possible parent candidates, we look if
      .. there are other rows further down (not yet added to check [])
      .. which is very similar to this row. So we can skip splitting
      .. this row. We use this, so there is no "def" chaining for
      .. for those future rows.
      */
      best = EMPTY;
    }


    /*
    .. We set the def [], and accept [] token of this state
    .. check[], base[] and next[] will be set inside "row_insert()".
    */
    def [s] = best;
    accept [s] = r->token;
    int loc = row_insert ( r, best, residual );
    if (loc == RGXERR) {
      error ("table compression : internal error");
      return RGXERR;
    }
    if (loc > offset) offset = loc;
  }

  deallocate (rows, (m+1)*sizeof (Row*));

  tsize [0][0] = tsize [0][1] = offset + n;
  tables [0][0] = check;  tables[0][1] = next;

  return 0;
}

#undef EMPTY
