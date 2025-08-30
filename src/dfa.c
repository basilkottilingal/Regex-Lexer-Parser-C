#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "nfa.h"
#include "regex.h"
#include "allocator.h"
#include "stack.h"

/*
.. Functions and objects required for creating a
.. DFA ( Deterministic Finite Automaton )
*/

typedef struct DState {
  Stack * list;
  struct DState * next [256];
  union {
    struct DState * child[2];
    struct DState * hchain;
  };
  Stack * bits;
  uint32_t hash;
  int i, flag;
} Dstate;

static int       nstates = 0;              /* Number of Dfa states. */

#define BITBYTES(_b) ( (_b + 63 & ~ (int) 63) >> 3 )

static DState ** htable  = NULL;                /* Uses a hashtable */
static int       hsize;                /* fixme : use prime numbers */
static int       stacksize;    /* bit stack size rounded to 8 bytes */

static DState * state ( Stack * list, Stack * bits, int * exists) {
  states_bstack (list, bits);
  uint32_t hash = stack_hash ((uint32_t *) bits->stack, bits->max);
  DState ** ptr = &htable [hash % hsize], * d;
  *exists = 1;
  while ( (d = *ptr) != NULL ) {          /* resolve hash collision */
    if (d->hash == hash && !stack_cmp (bits, d->bits))
      return d;
    ptr = &d->hchain;
  }
  *exists = 0;
  d = allocate ( sizeof (DState) );
  d->hash = hash;
  RGXMATCH (d) = RGXMATCH (list);
  d->list = stack_copy ( list );
  d->bits = stack_copy ( bits );
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
  int n = 0, exists, primes[] = /* prime > 2^N for N in { 6, 7, .. }*/
    { 67, 131, 257, 521, 1031, 2053, 4099, 8209, 16411 };
  while (++n) {
    if ( (1<<n) > RGXLIM ) { RTN (NULL); }                /* RGXOOM */
    if ( (1<<n) >= nnfa  )   break;
  }
  stacksize = BITBYTES (nnfa);        /* rounded as 64 bits multiple*/
  hsize = primes[ (++n < 6 ? 6 : n) - 6 ];
  htable = allocate ( hsize * sizeof (State *) );
  DState * root = state (list, bits, &exists);
  RTN (root);

  #undef RTN
}

/* ...................................................................
.. ...................................................................
.. ........  Algorithms related to DFA minimisation  .................
.. ...................................................................
.. ...................................................................
*/

/*
.. Traverse through the DFA tree starting from "root".
.. The full dfa set will be stored in *states = Q. And also in the
.. hashtable "htable", where the key is the bit set corresponding to
.. NFA Cache.
*/
static int
rgx_dfa_tree ( DState * root, Stack ** states ) {
  #define RTN(r)  stack_free (list); stack_free (bits);              \
    if (r<0) stack_free (Q); else *states = Q;                       \
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
      int c = s->id;
      if (c < 256 && dfa->next[c] == NULL ) {
        if (states_transition (dfa->list, list, buff, c) < 0) {
          RTN (RGXERR);
        }
        dfa->next[c] = state (list, bits, &exists);
        if (!exists) {   /* 'dfa' was recently pushed to hash table */
          dfa = dfa->next[c];
          if (n == RGXSIZE) { RTN (RGXOOM); }
          stack[n++] = (struct tree) { /* PUSH() & go down the tree */
            dfa, (State **) dfa->list->stack,
            dfa->list->len / sizeof (void *)
          };
        }
      }
    }

    do {
      dfa = stack[--n].d;  /* Pop a dfa from stack, add the dfa to Q*/
      dfa->i = state_id++;
      stack_push (Q, dfa);                     /* Set of all states */
    } while ( n && !stack[n-1].n );
  }

  RTN (0);

  #undef RTN
}

  #define BITCLEAR(B)       memset (B, 0, qsize);
  #define LOOKUP(B,b)                                                \
    ( B[(b)>>6] &  ((uint64_t) 1 << ((b) & (int) 63)))
  #define INSERT(B,b)                                                \
      B[(b)>>6] |= ((uint64_t) 1 << ((b) & (int) 63))
  #define MINUS(_u,_v,_w)                                            \
    for (int _i = 0; _i < (qsize>>3); ++_i) {                        \
      _w[_i] = _u[_i] & ~_v[_i];                                     \
    }
  #define AND(_u,_v,_w,_c)  do { _c = 0;                             \
    for (int _i = 0; _i < (qsize>>3); ++_i) {                        \
      _c |= (_w[_i] = _u[_i] & _v[_i]) != 0;                         \
    } } while (0)
  #define SAME(_u,_v,_c)    do { _c = 1;                             \
    for (int _i = 0; _c && _i < (qsize>>3); ++_i) {                  \
      _c = (_u[_i] == _v[_i]);                                       \
    } } while (0)
  #define POP(B)            ( B->len == 0 ? NULL :                   \
    ((uint64_t **) B->stack)                                         \
    [ (B->len -= sizeof (void *))/sizeof(void *) ] )
  #define PUSH(S,B)       stack_push (S, B)
  #if defined(__GNUC__) || defined(__clang__)
  #define COUNT(B,_c)       do { _c = 0;                             \
    for (int _i = 0; _i < (qsize>>3); ++_i) {                        \
      _c += __builtin_popcountll( B[_i] );                           \
    } } while (0)
  #else
  #define COUNT(B,c)        do { c = 0;                              \
    for (int _i = 0; _i < (qsize>>3); ++_i) {                        \
      uint64_t b64 = B[_i];                                          \
      while (b64) { b64 &= (b64-1); ++c;}                            \
    } } while (0)
  #endif
  #define FREE(B)         BITCLEAR (B); PUSH (pool, B)
  #define BSTACK(B)       uint64_t * B =                             \
    pool->len ? POP(pool) : allocate (qsize)
  #define STACK(S)        Stack * S = stack_new (0)
  #define COPY(S)                                                    \
    memcpy ( allocate (qsize), S, qsize )

static int dfa_minimal ( Stack * Q, Stack * P, DState ** dfa ) {

  /*
  .. New dfa set Q' from P.
  .. Use same hash table. Faster than O(n.n) bit comparison
  */
  *dfa = NULL;
  uint64_t i64, ** bitstack = (uint64_t **) P->stack, * Y, *y;
  uint32_t hash;
  int nq = Q->len/sizeof (void *), np = P->len/sizeof (void *),
    qsize = BITBYTES (nq);
  DState * d, ** next, * child,                        /* iterators */
    ** p =  (hsize < np) ? allocate ( np * sizeof (DState *) ) :
      memset ( htable, 0, np * sizeof (DState *) ), /* reuse htable */
    ** q = (DState **) Q->stack;                /* original dstates */
  Stack * cache = stack_new (0);

  for (int j=0; j<np; ++j) {                  /* each j in [0, |P|) */
    p[j] = d = allocate (sizeof (DState));             /* p[j] in P */

    stack_reset (cache);
    Y = bitstack[j] ;
    for (int k = 0; k < qsize >> 3; ++k) {         /* decoding bits */
      int base = k << 6, bit = 0;
      i64 = Y[k];
      while (i64) {
        #if defined(__clang__) || defined(__GNUC__)
        bit = __builtin_ctzll(i64);       /* position of lowest bit */
        #else
        if ( i64 & (uint64_t) 0x1 ) {
        #endif
          child = q [bit|base];
          if (child->i == nq-1)   /* root dfa is the TOP of Q stack */
            *dfa = d;
          child->i = j;        /* from now 'i' map each q[i] to p[j]*/
          printf(" pushed %d", bit|base);
          stack_push (cache, child);                /* Cache of q[i]*/
          stack_free (child->bits); child->bits = NULL;
        #if !defined(__clang__) && !defined(__GNUC__)
        }
        i64 >>= 1; ++bit;                      /* Iterate each bit. */
        #else
        i64 &= (i64 - 1);                   /* clear lowest set bit */
        #endif
      }
    }

    *d = (DState) {
      .list = stack_copy (cache),
      .i    = j
    };
  }
  stack_free (cache);

  /*
  .. Creating the transition for each p[i]
  */

  int c, n, m;
  Stack * tokens = stack_new (0);
  for (int j=0; j<np; ++j) {
    d = p[j];
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
          printf ("\n{d(%d,%c) = %d}", j, (char) c, s[n]->next[c]->i);
        }
        else if (c == NFAACC) {
          stack_push (tokens, nfa[m]);
        }
      }
      stack_free (s[n]->list); s[n]->list = NULL;
    }
    stack_free (cache); d->list = NULL;
    if (tokens->len) {
      d->list = stack_copy (tokens);
      stack_reset (tokens);
      RGXMATCH (d) = 1;
    }
  }
  stack_free (tokens);

  return (!*dfa) ? RGXERR : 1;
}

/*
.. Given an "old" DFA, it returns minimized DFA (*dfa)
*/

static int hopcroft ( State * nfa, DState ** dfa, int nnfa  ) {

  #define RTN(r) stack_free (Q); stack_free (P); return (r);

  STACK(pool);
  Stack * Q;
  DState * root = dfa_root (nfa, nnfa); if (!nfa) return RGXERR;
  if ( rgx_dfa_tree (root, &Q) < 0 || !Q )return RGXERR;
  int nq = Q->len / sizeof (void *), qsize = BITBYTES(nq);
  printf ("\n |Q| %d ", nq);
  BSTACK(F); BSTACK(Q_F);
  DState ** q = (DState **) Q->stack, * next;
  int count1 = 0, count2 = 0;
  for (int i=0; i<nq; ++i) {
    if (RGXMATCH (q[i])) {
      INSERT (F, i);  count1++; /* F := { q in Q | q is accepting } */
    }
    else {
      INSERT (Q_F, i); count2++;                             /* Q\F */
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
  char alphabets[] = "abcqd01"; /* fixme : use [0,256)*/
  while (W->len) {                                 /* while |W| > 0 */
    uint64_t * A = POP (W);                        /* A <- POP (W)  */
    int j = 0; char c;
    while ( (c = alphabets[j++]) != '\0') {        /* each c in Σ   */
      BITCLEAR (X);
      int k = 0;
      for (int i=0; i<nq; ++i) {
        if( (next = q[i]->next[c]) && LOOKUP(A, next->i) ) {
          k++; INSERT (X, i);      /* X <- { q in Q | δ(q,c) in A } */
        }
      }
      if (!k) continue;
      int nP = P->len/sizeof(void *);
      for (int i=0; i<nP; ++i) {                   /* each Y in P   */
        uint64_t ** y = (uint64_t **) P->stack,
          ** w = (uint64_t **) W->stack;
        uint64_t * Y = y [i];
        AND   ( Y, X, Y1, k );                     /* Y1 <- Y ∩ X   */
        if (!k) continue;                          /* Y ∩ X = ∅     */
        MINUS ( Y, X, Y2 );                        /* Y2 <- Y \ X   */
        COUNT (Y1, count1);                        /* |Y1|          */
        COUNT (Y2, count2);                        /* |Y2|          */
        if ( !count2 ) continue;                   /* Y \ X = ∅     */
        y [i] = COPY(Y1);
        PUSH ( P, COPY(Y2) );
        k = -1;
        int issame;
        for ( int l=0; l<W->len/sizeof(void *); ++l) {
          SAME ( Y, w[l], issame );
          if (issame) {                            /* if (Y ∈ W)    */
            FREE (w[l]); w[l] = COPY (Y1) ; PUSH (W, COPY(Y2));
            k = l; break;
          }
        }
        if (k<0)                                   /* if (Y ∉ W)    */
          PUSH ( W, count1 <= count2 ? COPY(Y1) : COPY(Y2) );
        FREE ( Y );
      }
    }
  }

  /*
  .. Partition of the set Q is now stored in P.
  .. (i)   P = {p_0, p_1, .. } with p_i ⊆ Q,
  .. (ii)  p_i ∩ p_j = ∅ iff i ≠ j
  .. (iii) collectively exhaustive, i.e,  union of p_i is Q
  */

  stack_free(W); stack_free (pool);
  int np = P->len/sizeof (void *); printf ("|Q'| %d", np);
  int rval = ( nq == np ) ? 0 :
    ( nq > np ) ?  dfa_minimal( Q, P, dfa ) :
    RGXERR;                              /* |P(Q)| should be <= |Q| */
  if (nq == np) *dfa = q[nq-1];
  RTN (rval);

  #undef RTN

}

  #undef BSTACK
  #undef LOOKUP
  #undef INSERT
  #undef SAME
  #undef AND
  #undef POP
  #undef PUSH
  #undef STACK
  #undef COUNT
  #undef MINUS
  #undef BITBYTES



int rgx_list_dfa ( Stack * list, DState ** dfa ) {
  int nr = list->len / sizeof (void *);
  char ** rgx = (char ** ) list->stack;
  State * nfa = allocate ( sizeof (State) ),
    ** out = allocate ( (nr+1) * sizeof (State *) );
  int n, nt = 0;
  state_reset ();
  for (int i=0; i<nr; ++i) {
    n = rgx_nfa (rgx[i], &out[i], i);
    if ( n < 0 ) return RGXERR;
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
  State * nfa = NULL;
  state_reset ();
  int n = rgx_nfa (rgx, &nfa, 0);
  if ( n < 0 ) return RGXERR;
  return hopcroft (nfa, dfa, n);
}

int rgx_dfa_match ( DState * dfa, const char * txt ) {
  const char *start = txt, *end = NULL;
  DState * d = dfa;
  //Stack * tokens = NULL;
  int c;
  if (RGXMATCH (d))  end = txt; // {tokens = d->list; }
  while ( (c = 0xFF & *txt++) ) {
    if ( d->next[c] == NULL )
      break;
    d = d->next[c];
    if (RGXMATCH (d))  end = txt; // { tokens = d->list; }
    printf ("\n[%c %d]", (char) c, end ? (int) (end-start) : -1 );
  }
  /*
  if(end) {
    int n = tokens->len / sizeof (void *);
    State ** f = (State **) tokens->stack;
    printf ("\n Token {");
    for (int i=0; i<n; ++i) 
      printf (" %d ", state_token (f[i]));
    printf ("}");
  }
  */
  /* return val = number of chars that match rgx + 1 */
  return end ? (int) ( end - start + 1) : 0;
}
