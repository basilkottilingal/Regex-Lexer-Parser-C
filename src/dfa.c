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
  struct DState * next [256];       /* for each transition, c in [0,256) */
  union {
    struct DState * child[2];       /* Binary tree, to maintain a database of DStates */
    struct DState * hchain;         /* Hash Chaining */
  };
  #ifdef RGXHSH
  Stack * bits;
  uint32_t hash;
  int i;
  #endif
  int flag;
} Dstate;

static DState *  root    = NULL;    /* Root of the btree datastructure */
static int       nstates = 0;       /* Number of Dfa states. */

#ifdef RGXHSH
#define BITBYTES(_b) ( (_b + 63 & ~ (int) 63) >> 3 )

static DState ** htable  = NULL;    /* Uses a hashtable */
static int       hsize;             /* fixme : use prime numbers */
static int       stacksize;         /* stacksize (bytes) . Rounded to nearest 8 multiple*/

static DState * state ( Stack * list, Stack * bits, int * exists) {
  states_bstack (list, bits);
  uint32_t hash = stack_hash ((uint32_t *) bits->stack, bits->max);
  DState ** ptr = &htable [hash % hsize], * d;
  *exists = 1;
  while ( (d = *ptr) != NULL ) {    /* resolve hash collision */
    if (d->hash == hash && !stack_bcmp (bits, d->bits))
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
#else

static DState * state ( Stack * list ) {
  stack_sort (list);
  DState ** ptr = &root, *d;
  while ( (d = *ptr) != NULL ) {
    int cmp = stack_pcmp (list, d->list);
    if(!cmp) return d;
    ptr = &d->child [cmp < 0 ? 0 : 1];
  }
  d = allocate ( sizeof (DState) );
  RGXMATCH (d) = RGXMATCH (list);
  d->list = stack_copy ( list );
  return (*ptr = d);
}

#endif

static DState * next ( DState * from, Stack * list,
  #ifdef RGXHSH
  Stack * bits,
  #endif
  State *** buff, int c )
{
  DState ** ptr = &from->next[c];
  if ( *ptr != NULL ) return *ptr;
  if ( states_transition ( from->list, list, buff, c) < 0) {
    return NULL;
  }
  #ifdef RGXHSH
  int exists;
  return ( *ptr = state (list, bits, &exists) );
  #else
  return ( *ptr = state (list) );
  #endif
}

int rgx_dfa ( char * rgx, DState ** dfa ) {

  State * nfa = NULL;
  Stack * list = stack_new (0);
  #ifdef  RGXHSH
  Stack * bits = NULL;
  #define RTN(r) stack_free (list); if(bits) stack_free (bits); return r
  #else
  #define RTN(r) stack_free (list); return r
  #endif

  int nnfa = rgx_nfa (rgx, &nfa);                   /* nfa graph */
  if (nnfa <= 0) { RTN (RGXERR); }

  State ** buff[RGXSIZE];
  int status = states_at_start ( nfa, list, buff ); /* ε-closure @ start */
  if (status < 0) { RTN (status); }

  nstates = 0;                                      /* fixme : Cleaned previous nodes?*/
  #ifndef RGXHSH
  root = NULL;
  *dfa = state (list);
  #else
  int n = 0, exists, primes[] =                     /* prime > 2^N for
                                                    .. N in  { 6, 7, 8,.. } */
    { 67, 131, 257, 521, 1031, 2053, 4099, 8209, 16411 };
  while (++n) {
    if ( (1<<n) > RGXLIM ) return RGXOOM;
    if ( (1<<n) >= nnfa )  break;
  }
  stacksize = BITBYTES (nnfa);                  /* rounded as 64 bits multiple*/
  hsize = primes[ (++n < 6 ? 6 : n) - 6 ];
  htable = allocate ( hsize * sizeof (State *) );   /* fixme : allocate() lmtd to 8kB */
  bits = stack_new (stacksize);
  *dfa = state (list, bits, &exists);
  #endif
  RTN ( *dfa != NULL ? 0 : RGXERR );
}

int rgx_dfa_match ( DState * dfa, const char * txt ) {

  State ** buff[RGXSIZE];
  Stack * list = stack_new (0);
  #ifdef RGXHSH
  Stack * bits = stack_new (stacksize);
  #endif
  const char *start = txt;
  DState * d = dfa;
  int status, c, accept = RGXMATCH (d);

  while ( (c = 0xFF & *txt++) ) {
    d = next (d, list,
              #ifdef RGXHSH
              bits,
              #endif
              buff, c);
    if (!d)                          {  RTN (RGXERR); }
    if (RGXMATCH (d))                {  accept = 1; continue; }
    if (accept || !d->list->nentries)   break;
  }
  /* return val = number of chars that match rgx + 1 */
  RTN (accept ? (int) ( txt - start ) : 0);

  #undef RTN
}

/* ..................................................................
.. ..................................................................
.. .......  Algorithms related to DFA minimisation  .................
.. ..................................................................
.. ..................................................................
*/

#ifdef RGXHSH

static int
rgx_dfa_tree ( char * rgx, Stack ** states ) {
  #define RTN(r)  stack_free (list); stack_free (bits);           \
    if (r<0) stack_free (Q); else *states = Q;                    \
    return r

  int c, exists = 0;
  struct tree {
    DState * d; State ** s; int n;
  } stack [ RGXSIZE ];
  State ** buff[RGXSIZE], *s;
  Stack * list = stack_new (0),
    * bits = stack_new (stacksize),
    * Q = stack_new(0);             /* Q   : set of DFA nodes */

  DState * dfa = NULL;
  if (rgx_dfa (rgx, &dfa) < 0) { RTN (RGXERR); }
  stack[0] = (struct tree) {
    dfa, (State **) dfa->list->stack, dfa->list->len / sizeof (void *)
  };
  int state_id = 0, n = 1;                        /* Depth of the tree */
  while (n) {

    /* Go down the tree if 'next' dfa node is a newly created node */
    while ( (s = stack[n-1].n ? stack[n-1].s[--stack[n-1].n] : NULL) ) {
                                    /* iterate each nfa of dfa cache */
      dfa = stack[n-1].d;
      int c = s->id;
      if (c < 256 && dfa->next[c] == NULL ) {
        if (states_transition (dfa->list, list, buff, c) < 0) {
          RTN (RGXERR);
        }
        dfa->next[c] = state (list, bits, &exists);
        if (!exists) {              /* This 'd' was recently pushed to hash table */
          dfa = dfa->next[c];
                                    /* PUSH() or go down the tree */
          if (n == RGXSIZE) { RTN (RGXOOM); }
          stack[n++] = (struct tree) {
            dfa, (State **) dfa->list->stack,
            dfa->list->len / sizeof (void *)
          };
        }
      }
    }

    do {
      /* Pop a dfa from tree stack and add the dfa to Q := {q} */
      dfa = stack[--n].d;
      dfa->i = state_id++;
      stack_push (Q, dfa);          /* Set of all states */
    } while ( n && !stack[n-1].n );
  }

  RTN (0);

  #undef RTN
}

DState * FULL_DFA ( char * rgx ) {
  Stack * states;// = allocate (3*sizeof (Stack *));
  rgx_dfa_tree (rgx, &states);
  /* root DFA will be at the top of the Q stack */
  int n = states->len/sizeof(void *);
  return ((DState **) states->stack ) [n - 1];
}

int FULL_DFA_PATTERN ( DState * dfa, const char * txt ) {
  const char *start = txt;
  DState * d = dfa;
  int accept = RGXMATCH (d), c;
  while ( (c = 0xFF & *txt++) ) {
    if ( d->next[c] == NULL )
      break;
    d = d->next[c];
    accept = RGXMATCH (d);
  }
  /* return val = number of chars that match rgx + 1 */
  return accept ? (int) ( txt - start ) : 0;
}

  #define BITCLEAR(B)       memset (B, 0, qsize);
  #define LOOKUP(B,b)                                     \
    ( B[(b)>>6] &  ((uint64_t) 1 << ((b) & (int) 63)))
  #define INSERT(B,b)                                     \
      B[(b)>>6] |= ((uint64_t) 1 << ((b) & (int) 63))
  #define MINUS(_u,_v,_w)                                 \
    for (int _i = 0; _i < (qsize>>3); ++_i) {             \
      _w[_i] = _u[_i] & ~_v[_i];                          \
    }
  #define AND(_u,_v,_w,_c)  do { _c = 0;                  \
    for (int _i = 0; _i < (qsize>>3); ++_i) {             \
      _c |= (_w[_i] = _u[_i] & _v[_i]) != 0;              \
    } } while (0)
  #define SAME(_u,_v,_c)    do { _c = 1;                  \
    for (int _i = 0; _c && _i < (qsize>>3); ++_i) {       \
      _c = (_u[_i] == _v[_i]);                            \
    } } while (0)
  #define POP(B)            ( B->len == 0 ? NULL :        \
    ((uint64_t **) B->stack)                              \
    [ (B->len -= sizeof (void *))/sizeof(void *) ] )
  #define PUSH(S,B)       stack_push (S, B)
  #if defined(__GNUC__) || defined(__clang__)
  #define COUNT(B,_c)       do { _c = 0;                  \
    for (int _i = 0; _i < (qsize>>3); ++_i) {             \
      _c += __builtin_popcountll( B[_i] );                \
    } } while (0)
  #else
  #define COUNT(B,c)        do { c = 0;                   \
    for (int _i = 0; _i < (qsize>>3); ++_i) {             \
      uint64_t b64 = B[_i];                               \
      while (b64) { b64 &= (b64-1); ++c;}                 \
    } } while (0)
  #endif
  #define FREE(B)         BITCLEAR (B); PUSH (pool, B)
  #define BSTACK(B)       uint64_t * B =                  \
    pool->len ? POP(pool) : allocate (qsize)
  #define STACK(S)        Stack * S = stack_new (0)
  #define COPY(S)                                         \
    memcpy ( allocate (qsize), S, qsize )

static int
DFA_MINIMAL ( Stack * Q, Stack * P, DState ** dfa ) {

  /* 
  .. New dfa set Q' from P. 
  .. Use same hash table. Faster than O(n.n) bit comparison
  */
  *dfa = NULL;
  uint64_t i64, ** bitstack = (uint64_t **) P->stack, * Y, *y;
  uint32_t hash;
  int nq = Q->len/sizeof (void *), np = P->len/sizeof (void *),
    qsize = BITBYTES (nq), accept;
  DState * d, ** next, * child,                                /* iterators */
    ** p =  (hsize < np) ? allocate ( np * sizeof (DState *) ) :   /* {p} */
      memset ( htable, 0, np * sizeof (DState *) ),              /* reuse */
    ** q = (DState **) Q->stack;                      /* original dstates */
  Stack * cache = stack_new (0);  

  for (int j=0; j<np; ++j) {                        /* each j in [0, |P|) */
    accept = 0;
    p[j] = d = allocate (sizeof (DState));                   /* p[j] in P */

    stack_reset (cache);
    Y = bitstack[j] ;
    for (int k = 0; k < qsize >> 3; ++k) {               /* decoding bits */
      int base = k << 6, bit = 0;
      i64 = Y[k];
      while (i64) {
        #if defined(__clang__) || defined(__GNUC__)
        bit = __builtin_ctzll(i64);             /* position of lowest bit */
        #else
        if ( i64 & (uint64_t) 0x1 ) {
        #endif
          child = q [bit|base];
          accept |= RGXMATCH (child);
          if (child->i == nq-1)      /* root dfa is at the top of Q stack */
            *dfa = d;
          child->i = j;              /* from now 'i' map each q[i] to p[j]*/
          printf(" pushed %d", bit|base);
          stack_push (cache, child);                      /* Cache of q[i]*/
          stack_free (child->bits); child->bits = NULL;
        #if !defined(__clang__) && !defined(__GNUC__)
        }
        i64 >>= 1; ++bit;                            /* Iterate each bit. */
        #else
        i64 &= (i64 - 1);                         /* clear lowest set bit */
        #endif
      }
    }

    *d = (DState) { 
      .list = stack_copy (cache),
      .i    = j,
    }; 
    RGXMATCH (d) = accept;
  }
  stack_free (cache);

  /*
  .. Creating the 
  .. (a) transition for p[i], 
  .. (b) F' = { p[i] | p[i] ∩ F ≠ ∅ } and 
  .. (c) starting node : unique p[k] with, q[0] ∈ p[k] 
  */
   
  int c, n, m;
  for (int j=0; j<np; ++j) {
    cache = p[j]->list;
    DState ** s = (DState **) cache->stack;
    n = cache->len / sizeof (void *);
    next = p[j]->next;
    while (n--) {
      State ** nfa = (State **) s[n]->list->stack;
      m = s[n]->list->len / sizeof (void *);
      while (m--) {
        if ( (c = nfa[m]->id) < 256 && c >= 0 && !next[c] ) {
          if (s[n]->next[c] == NULL) return RGXERR;
          next[c] = p [ s[n]->next[c]->i ];
          printf (" {d(%d,%c) = %d}", j, (char) c, s[n]->next[c]->i);
        }
      }
      stack_free (s[n]->list); s[n]->list = NULL;
    }
    stack_free (cache); p[j]->list = NULL;
  }
  return (!*dfa) ? RGXERR : 1;
}

int DFA_MINIMIZATION ( char * rgx, DState ** dfa ) {

  #define RTN(r) stack_free (Q); stack_free (P); return (r);

  STACK(pool);
  Stack * Q;
  rgx_dfa_tree (rgx, &Q);  if (!Q) return RGXERR;
  int nq = Q->len / sizeof (void *), qsize = BITBYTES(nq);
  printf ("\n |Q| %d ", nq);
  BSTACK(F); BSTACK(Q_F);
  DState ** q = (DState **) Q->stack, * next;
  for (int i=0; i<nq; ++i) {
    if (RGXMATCH (q[i])) 
      INSERT (F, i);                         /* F := { accepting states } */
    else INSERT (Q_F, i);                                          /* Q\F */
  }

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


  STACK(P); PUSH (P, Q_F); PUSH (P, F);                   /* P <- {F, Q\F} */
  STACK(W); PUSH (W, F);                                  /* W <- {F}      */
  BSTACK (X); BSTACK (Y1); BSTACK (Y2);
  char alphabets[] = "abcqd01"; /* fixme : use [0,256)*/
  while (W->len) {                                        /* while |W| > 0 */
    uint64_t * A = POP (W);                               /* A <- POP (W)  */
    int j = 0; char c;
    while ( (c = alphabets[j++]) != '\0') {               /* each c in Σ   */
      BITCLEAR (X);
      int k = 0;
      for (int i=0; i<nq; ++i) {
        if( (next = q[i]->next[c]) && LOOKUP(A, next->i) ) {
          k++; INSERT (X, i);             /* X <- { q in Q | δ(q,c) in A } */
        }
      }
      if (!k) continue;
      int nP = P->len/sizeof(void *);
      for (int i=0; i<nP; ++i) {                          /* each Y in P   */
        uint64_t ** y = (uint64_t **) P->stack,
          ** w = (uint64_t **) W->stack;
        uint64_t * Y = y [i];
        AND   ( Y, X, Y1, k );                            /* Y1 <- Y ∩ X   */
        if (!k) continue;                                 /* Y ∩ X = ∅     */
        MINUS ( Y, X, Y2 );                               /* Y2 <- Y \ X   */
        int count1, count2;
        COUNT (Y1, count1);                               /* |Y1|          */
        COUNT (Y2, count2);                               /* |Y2|          */
        if ( !count2 ) continue;                          /* Y \ X = ∅     */
        y [i] = COPY(Y1);
        PUSH ( P, COPY(Y2) );
        k = -1;
        int issame;
        for ( int l=0; l<W->len/sizeof(void *); ++l) {
          SAME ( Y, w[l], issame );
          if (issame) {                                   /* if (Y ∈ W)    */
            FREE (w[l]); w[l] = COPY (Y1) ; PUSH (W, COPY(Y2));
            k = l; break;
          }
        }
        if (k<0)                                          /* if (Y ∉ W)    */
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
    ( nq > np ) ? DFA_MINIMAL ( Q, P, dfa ) :
    RGXERR;                                    /* |P(Q)| should be <= |Q| */
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
#endif
