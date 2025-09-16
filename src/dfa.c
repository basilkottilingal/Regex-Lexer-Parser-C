#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "nfa.h"
#include "class.h"
#include "regex.h"
#include "allocator.h"
#include "stack.h"
#include "bits.h"

/*
.. Functions and objects required for creating a
.. DFA ( Deterministic Finite Automaton )
*/

typedef struct DState {
  Stack * list;
  struct DState ** next;
  struct DState * hchain;
  Stack * bits;
  int i, flag;
  int token;                              /* preferred token number */
  uint32_t hash;
} Dstate;

static DState ** states = NULL;               /* list of dfa states */
static int       nstates = 0;              /* Number of Dfa states. */
static DState ** htable  = NULL;                /* Uses a hashtable */
static int       hsize;                /* fixme : use prime numbers */
static int       stacksize;    /* bit stack size rounded to 8 bytes */
static int     * class = NULL;
static int       nclass = 0;

static DState * state ( Stack * list, Stack * bits, int * exists) {
  states_bstack (list, bits);
  uint32_t hash = stack_hash ((uint32_t *) bits->stack, bits->max);
  DState ** ptr = & htable [hash % hsize], * d;
  *exists = 1;
  while ( (d = *ptr) != NULL ) {          /* resolve hash collision */
    if (d->hash == hash && !stack_cmp (bits, d->bits))
      return d;
    ptr = & d->hchain;
  }
  *exists = 0;
  d = allocate ( sizeof (DState) );
  *d = (DState) {
    .next = allocate ( nclass * sizeof (DState *)),
    .hash = hash,
    .flag = RGXMATCH (list),
    .list = stack_copy ( list ),
    .bits = stack_copy ( bits )
  };
  return (*ptr = d);
}

static DState * dfa_root ( State * nfa, int nnfa ) {

  Stack * list = stack_new (0);
  Stack * bits = stack_new (stacksize);
  #define RTN(r) stack_free (list); stack_free (bits); return r

  State ** buff[RGXSIZE];
  int status = states_at_start ( nfa, list, buff );
  if (status < 0) { RTN (NULL); }

  nstates = 0;                   /* fixme : Cleaned previous nodes? */
  int n = 6, exists, primes[] = /* prime > 2^N for N in { 6, 7, .. }*/
    { 67, 131, 257, 521, 1031, 2053, 4099, 8209, 16411 };
  while ((1<<n) < nnfa) n++;
  stacksize = BITBYTES (nnfa);        /* rounded as 64 bits multiple*/
  hsize = primes[ n - 6 ];
  while (hsize * sizeof (void *) > PAGE_SIZE)       /* memory limit */
    hsize = primes[ --n - 6 ];
  htable = allocate ( hsize * sizeof (State *) );
  DState * root = state (list, bits, &exists);
  RTN (root);

  #undef RTN
}

/* ...................................................................
.. ...................................................................
.. ........  Algorithms related to DFA minimisation  .................
.. ...................................................................
.. .................................................................*/

/*
.. Traverse through the DFA tree starting from "root".
.. The full dfa set will be stored in *states = Q. And also in the
.. hashtable "htable", where the key is the bit set corresponding to
.. NFA Cache.
*/
static int
rgx_dfa_tree ( DState * root, Stack ** Qptr ) {
  #define RTN(r)  stack_free (list); stack_free (bits);              \
    if (r<0) stack_free (Q); else *Qptr = Q;                         \
    return r

  DState * dfa;
  int c, exists = 0;
  struct tree {
    DState * d; State ** s; int n;
  } stack [ RGXSIZE ];
  State ** buff[RGXSIZE], *s;
  Stack * list = stack_new (0),
    * bits = stack_new (stacksize),
    * Q = stack_new(0);                   /* Q   : set of DFA nodes */

  stack[0] = (struct tree) {
    root, (State **) root->list->stack,
    root->list->len / sizeof (void *)
  };
  int state_id = 0, n = 1;                     /* Depth of the tree */
  while (n) {

    /*
    .. Go down the tree if 'next' dfa node is a newly created node
    */
    while ((s = stack[n-1].n ? stack[n-1].s[--stack[n-1].n] : NULL)) {
                                   /* iterate each nfa of dfa cache */
      dfa = stack[n-1].d;
      c = s->id;
      if (c >= 256 || dfa->next[c] ) continue;

      if (states_transition (dfa->list, list, buff, c) < 0) {
          RTN (RGXERR);
      }
      dfa->next[c] = state (list, bits, &exists);
      if (!exists) {     /* 'dfa' was recently pushed to hash table */
        dfa = dfa->next[c];
        if (n == RGXSIZE) { RTN (RGXOOM); }
        stack[n++] = (struct tree) {   /* PUSH() & go down the tree */
          dfa, (State **) dfa->list->stack,
          dfa->list->len / sizeof (void *)
        };
      }
    }

    do {
      dfa = stack[--n].d; /* Pop a dfa from stack, add the dfa to Q */
      dfa->i = state_id++;
      stack_push (Q, dfa);                     /* Set of all states */
    } while ( n && !stack[n-1].n );
  }

  states = (DState **) Q->stack;
  RTN (0);

  #undef RTN
}

  #define POP(B)          ( B->len == 0 ? NULL :                     \
    ((uint64_t **) B->stack)                                         \
    [ (B->len -= sizeof (void *))/sizeof(void *) ] )
  #define PUSH(S,B)       stack_push (S, B)
  #define FREE(B,s)       BITCLEAR (B,s); PUSH (pool, B)
  #define BSTACK(B)       uint64_t * B =                             \
    pool->len ? POP(pool) : allocate (qsize)
  #define STACK(S)        Stack * S = stack_new (0)
  #define COPY(S)                                                    \
    memcpy ( allocate (qsize), S, qsize )

static int dfa_minimal ( Stack * Q, Stack * P, DState ** dfa ) {

  printf ("\n dfa minimal |p|%zd", P->len/sizeof(void *)); fflush (stdout);
  /*
  .. New dfa set Q' from P.
  .. Use same hash table. Faster than O(n.n) bit comparison
  */
  uint64_t i64, ** bitstack = (uint64_t **) P->stack, * Y;
  int nq = Q->len/sizeof (void *), np = P->len/sizeof (void *),
    qsize = BITBYTES (nq);
  DState * d, ** next, * child,                        /* iterators */
    ** p =  (hsize < np) ? allocate ( np * sizeof (DState *) ) :
      memset ( htable, 0, np * sizeof (DState *) ), /* reuse htable */
    ** q = (DState **) Q->stack,                /* original dstates */
    * _q0 = NULL;
  Stack * cache = stack_new (0);

  for (int j=0; j<np; ++j) {                  /* each j in [0, |P|) */
    p[j] = d = allocate (sizeof (DState));             /* p[j] in P */

    stack_reset (cache);
    Y = bitstack[j] ;
    for (int k = 0; k < qsize >> 3; ++k) {         /* decoding bits */
      int base = k << 6, bit = 0;
      i64 = Y[k];
      while (i64) {
        bit = __builtin_ctzll(i64);       /* position of lowest bit */

        child = q [bit|base];
        if (child->i == nq-1)     /* root dfa is the TOP of Q stack */
          _q0 = d;                   /* root of the new DFA tree Q' */
        child->i = j;          /* from now 'i' map each q[i] to p[j]*/
        stack_push (cache, child);                  /* Cache of q[i]*/
        stack_free (child->bits); child->bits = NULL;

        i64 &= (i64 - 1);                   /* clear lowest set bit */
      }
    }

    *d = (DState) {
      .list = stack_copy (cache),
      .i    = j,
      .next = allocate ( nclass * sizeof (DState *))
    };
  }
  stack_free (cache);

  if(!_q0) return RGXERR;
  *dfa = _q0;

  /*
  .. (b) Creating the transition for each p[i]
  .. (b) Identifying accepting states of Q'. Lowest token number
  ..     will be assigned to q->token for all q in Q'.
  ..     ( Assumes : lower the itoken, more the precedence )
  */

  int c, n, m, token;
  for (int j=0; j<np; ++j) {
    d = p[j];
    d->token = INT_MAX;
    RGXMATCH (d) = 0;
    cache = d->list;
    DState ** s = (DState **) cache->stack;
    n = cache->len / sizeof (void *);
    next = d->next;
    while (n--) {
      State ** nfa = (State **) s[n]->list->stack;
      m = s[n]->list->len / sizeof (void *);
      while (m--) {
        if ( (c = nfa[m]->id) < 256 && c >= 0 && !next[c] ) {
          if (s[n]->next[c] == NULL) return RGXERR;
          next[c] = p [ s[n]->next[c]->i ];
        }
        else if (c == NFAACC) {
          RGXMATCH (d) = 1;
          token = state_token ( nfa[m] );
          if (token < d->token)     /* assumes itoken in decr order */
            d->token = token;
        }
      }
      stack_free (s[n]->list); s[n]->list = NULL;
    }
    stack_free (cache); d->list = NULL;
  }

  states = p;
  nstates = np;
  return 1;
}

#if 1
void delete (Stack * P, int qsize) {
  int _np = P->len / sizeof (void *);
  uint64_t ** _bits = (uint64_t **) P->stack;
  int _nbits;
  printf ("\n{ ");
  for(int i=0; i<_np;++i) {
    BITCOUNT(_bits[i], _nbits, qsize); 
    printf ("%d, ", _nbits);
  }
  printf ("}");
}
#endif

/*
.. Given a "root" NFA, it returns minimized DFA (*dfa)
*/

static int hopcroft ( State * nfa, DState ** dfa, int nnfa  ) {

  #define RTN(r) stack_free (Q); stack_free (P); return (r);

  STACK(pool);
  Stack * Q;
  DState * root = dfa_root (nfa, nnfa); if (!nfa) return RGXERR;
  if ( rgx_dfa_tree (root, &Q) < 0 || !Q )return RGXERR;
  int nq = Q->len / sizeof (void *), qsize = BITBYTES(nq);
  printf ("\n |Q| %d ", nq); fflush (stdout);
  BSTACK(F); BSTACK(Q_F);
  DState ** q = (DState **) Q->stack, * next;
  int count1 = 0, count2 = 0;
  for (int i=0; i<nq; ++i) {
    if (RGXMATCH (q[i])) {
      BITINSERT (F, i); count1++; /* F := { q in Q |q is accepting }*/
    }
    else {
      BITINSERT (Q_F, i); count2++;                          /* Q\F */
    }
  }
  if (!count1) return RGXERR;

  /*
  ..  function hopcroft(DFA):
  ..    P := { F , Q \ F }
  ..    W := { F }
  ..
  ..    while W not empty:
  ..      A := remove some element from W
  ..      for each input symbol c:
  ..        X := { q in Q | δ(q,c) in A }
  ..        for each block Y in P that intersects X:
  ..          Y1 := Y ∩ X
  ..          Y2 := Y \ X
  ..          replace Y in P with Y1, Y2
  ..          if Y in W:
  ..            replace Y with Y1, Y2 in W
  ..          else:
  ..            add smaller of (Y1,Y2) to W
  ..    return P
  */

  STACK(P); if(count2) PUSH (P, Q_F); PUSH (P, F); /* P <- {F, Q\F} */
  STACK(W); PUSH (W, F);                           /* W <- {F}      */
  BSTACK (X); BSTACK (Y1); BSTACK (Y2);
  while (W->len) {                                 /* while |W| > 0 */
    uint64_t * A = POP (W);                        /* A <- POP (W)  */
    int c = nclass;
    while ( c-- ) {                          /* each c in [0, P(Σ)) */
      BITCLEAR (X, qsize);
      int k = 0;
      for (int i=0; i<nq; ++i) {
        if( (next = q[i]->next[c]) && BITLOOKUP (A, next->i) ) {
          k++; BITINSERT (X, i);   /* X <- { q in Q | δ(q,c) in A } */
        }
      }
      if (!k) continue;
      int nP = P->len/sizeof(void *);
      for (int i=0; i<nP; ++i) {                   /* each Y in P   */
        uint64_t ** y = (uint64_t **) P->stack,
          ** w = (uint64_t **) W->stack;
        uint64_t * Y = y [i];
        BITAND ( Y, X, Y1, k, qsize );             /* Y1 <- Y ∩ X   */
        if (!k) continue;                          /* Y ∩ X = ∅     */
        BITMINUS ( Y, X, Y2, qsize );              /* Y2 <- Y \ X   */
        BITCOUNT ( Y1, count1, qsize );            /* |Y1|          */
        BITCOUNT ( Y2, count2, qsize );            /* |Y2|          */
        if ( !count2 ) continue;                   /* Y \ X = ∅     */
        y [i] = COPY (Y1);
        PUSH ( P, COPY (Y2) );
        k = -1;
        int issame;
        for ( int l=0; l<W->len/sizeof(void *); ++l) {
          BITCMP ( Y, w[l], issame, qsize );
          if (issame) {                            /* if (Y ∈ W)    */
            /*FREE (w[l]);*/ w[l] = COPY (Y1) ; PUSH (W, COPY(Y2));
            k = l; break;
          }
        }
        if (k<0)                                   /* if (Y ∉ W)    */
          PUSH ( W, count1 <= count2 ? COPY(Y1) : COPY(Y2) );
        /*FREE ( Y );*/
      }
    }
  }

  /*
  .. Partition of the set Q is now stored in P.
  .. (i)   P = {p_0, p_1, .. } with p_i ⊆ Q, and p_i ≠ ∅
  .. (ii)  p_i ∩ p_j = ∅ iff i ≠ j
  .. (iii) collectively exhaustive, i.e,  union of p_i is Q
  */
  stack_free(W); stack_free (pool);
  int np = P->len/sizeof (void *); printf ("|Q'| %d", np);
  dfa [0] = q[nq-1];
  int rval = ( nq == np ) ? 0 :
    ( nq > np ) ?  dfa_minimal( Q, P, dfa) :
    RGXERR;                              /* |P(Q)| should be <= |Q| */
  RTN (rval);

  #undef RTN

}

  #undef BSTACK
  #undef POP
  #undef FREE
  #undef PUSH
  #undef STACK
  #undef COPY

int rgx_list_dfa ( char ** rgx, int nr, DState ** dfa ) {
  int n, nt = 0;
  State * nfa = allocate ( sizeof (State) ),
    ** out = allocate ( (nr+1) * sizeof (State *) );
  nfa_reset ( rgx, nr );
  class_get ( &class, &nclass );
  for (int i=0; i<nr; ++i) {
    /*
    .. Note that the token number itoken = 0, is reserved for error
    */
    n = rgx_nfa (rgx[i], &out[i], i+1 );
    if ( n < 0 ) {
      error ("rgx list nfa : cannot create nfa for rgx \"%s\"", rgx);
      return RGXERR;
    }
    nt += n;
  }
  *nfa = (State) {
    .id  = NFAEPS,
    .ist = nt++,
    .out = out
  };

  return hopcroft (nfa, dfa, nt);
}

int rgx_dfa ( char * rgx, DState ** dfa ) {
  return rgx_list_dfa (&rgx, 1, dfa);
}

int rgx_dfa_match ( DState * dfa, const char * txt ) {
  const char *start = txt, *end = NULL;
  DState * d = dfa;
  int c;
  if (RGXMATCH (d))  end = txt;
  while ( (c = 0xFF & *txt++) ) {
    if ( (d = d->next[class[c]]) == NULL )
      break;
    if (RGXMATCH (d)) end = txt;
  }
  /* return val = number of chars that match rgx + 1 */
  return end ? (int) (end - start + 1) : 0;
}

/* ...................................................................
.. ...................................................................
.. ........  Algorithms related to table compression .................
.. ...................................................................
.. .................................................................*/

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

typedef struct Row {
  int s, n, start, span;
} Row;

static int compare ( const void * a, const void * b ) {
  #define CMP(p,q) cmp = ((int) (p > q) - (int) (p < q));            \
    if (cmp) return cmp
  Row * s = (Row *)a, * r = (Row *)b;
  int cmp;                  /* sort by                              */
  CMP (r->n, s->n);         /* number of transitions (decreasing)   */
  CMP (s->span, r->span);   /* span = end-start  (increasing)       */
  CMP (s->start, r->start); /* first transition  (lowest preferred) */
  CMP (s->s, r->s);         /* state id (lowest preferred)          */
  return 0;
  #undef CMP
}

static void resize (int ** check, int ** next, int * k, int k0){
  #define EMPTY -1
  int sold = (*k) * sizeof (int), s = sold + k0 * sizeof(int);
  *check = reallocate (*check, sold, s);
  *next = reallocate (*next, sold, s);
  memset (& (*check) [*k], EMPTY, s - sold);
  (*k) += k0;
}

static Row * rows_ordered () {
  int m = nstates, n = nclass;
  Row * rows = allocate ( (m+1) * sizeof (Row)), * row = rows;
  for (int s=0; s<m; ++s, ++row) {
    int ntrans = 0, start = EMPTY, end = EMPTY;
    DState ** d = states[s]->next;
    for (int c = 0; c < n; ++c)
      if (d[c]) {
        ntrans++; end = c;
        if (start == EMPTY) start = c;
      }
    *row = (Row) {
      .n = ntrans, .span = end - start, .start = start, .s = s
    };
  }
  row->s = EMPTY;
  qsort (rows, m, sizeof (Row), compare);
  return rows;
}

int dfa_tables (int *** tables, int ** tsize) {

  int ** t = * tables = allocate ( 5 * sizeof (int *) );
  int * len = * tsize = allocate ( 5 * sizeof (int) );

  int m = nstates, n = nclass,
    k0 = 4 * n;     /* let's start with k=4n & reallocate if needed */
  k0 = 1 << (64 - __builtin_clzll ((unsigned long long)(k0 - 1)) );
  int k = k0, stack [257];

  int * check = allocate ( k* sizeof (int)),
    * next = allocate (k * sizeof(int)),
    * base = allocate (m * sizeof (int)),
    * accept = allocate (m * sizeof (int));

  memset ( check, EMPTY, k * sizeof (int) );


  int offset = 0, s;
  /* Order rows by density */
  Row * rows = rows_ordered (), * row = rows;
  #if 0
  Row * _row = rows;
  while ( (s = (*_row++).s ) != EMPTY ) {
    printf ("\ns%2d n%2d start%2d span%2d",
    _row[-1].s, _row[-1].n, _row[-1].start, _row[-1].span);
  }
  #endif
  while ( (s = (*row++).s ) != EMPTY ) {
    if (offset + 2*n > k)          /* resize next and check if reqd */
      resize (&check, &next, &k, k0);

    if (row[-1].n == 1) {
      /*
      .. Place all the states with single character differently.
      .. Look for the empty slots beginning from index 0
      */
      int slot = 0, c;
      do {
        DState * q = states [s], ** d = q->next;
        c = row[-1].start;
        while ( check [slot] != EMPTY || slot - c < 0){
          ++slot;
          if (slot + n > k) {
            resize (&check, &next, &k, k0);
          }
        }
        next [slot] = d[c]->i;
        check [slot] = s;
        accept [s] = RGXMATCH (q) ? q->token : 0;
        base [s] = slot - c;
        if ( base [s] > offset )
          offset = base [s];
      } while ( (s = row->s) != EMPTY && (*row++).n );

      /*
      .. states with zero transitions. place somewhere
      */

      if (s != EMPTY) do {
        DState * q = states [s];
        base [s] = 0;
        accept [s] = RGXMATCH (q) ? q->token : 0;
      } while ( (s = (*row++).s) != EMPTY );
      break;
    }

    DState * q = states [s], ** d = q->next;

    int * ptr = stack, c;
    for (c=0; c<n; ++c)                            /* each eq class */
      if ( d[c] )       /* stack transitions excepts DEAD transition*/
        *ptr++ = c;
    *ptr = EMPTY;

    ptr = stack;
    while ( (c=*ptr++) != EMPTY ) {
      if ( check [offset + c] != EMPTY ) {            /* collision. */
        ptr = stack;
        offset ++;                    /* restart, with a new offset */
      }
    }

    ptr = stack;
    while ( (c=*ptr++) != EMPTY ) {
      next [offset + c] = d[c]->i;
      check [offset + c] = s;                         /* insert row */
    }
    accept [s] = RGXMATCH (q) ? q->token : 0;
    base [s] = offset;
  }

  deallocate (rows, (m+1)*sizeof (Row));

  len [0] = len [1] = offset + n;
  len [2] = len [3] = m;
  len [4] = 256;

  t [0] = check;  t [1] = next;  t [2] = base;
  t [3] = accept; t [4] = class;

  return 0;
  #undef EMPTY
}
