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
  struct DState * next [256];
  struct DState * hchain;
  Stack * bits;
  int i, flag;
  int token;                              /* preferred token number */
  uint32_t hash;
} Dstate;

static int       nstates = 0;              /* Number of Dfa states. */

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
      c = s->id;
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
      .i    = j
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
          token = state_token (nfa[m]);
          if (token < d->token)     /* assumes itoken in decr order */
            d->token = token; 
        }
      }
      stack_free (s[n]->list); s[n]->list = NULL;
    }
    stack_free (cache); d->list = NULL;
  }

  return 1;
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
  char alphabets[] = "abcqd01"; /* fixme : use [0,256)*/
  while (W->len) {                                 /* while |W| > 0 */
    uint64_t * A = POP (W);                        /* A <- POP (W)  */
    int j = 0, c;
    while ( (c = (int) alphabets[j++]) != '\0') {  /* each c in Σ   */
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

int equivalence_class ( Stack * rgxlist ) {
  int csingle [256], cstack [256],      /* For single char eq class */
    nc = 0, group [256], ig,          /* For charcter set [], [^..] */
    nr = rgxlist->len / sizeof (void *),  /* number of rgx pattersn */
    charclass = 0, escape = 0, temp = 0; 
  char ** ptr = (char **) rgxlist->stack, *rgx;
  memset (csingle, 0, sizeof (csingle));

  while (nr--) {
    rgx = ptr [nr];
  }

  for (int j=0; j<nc; ++j ) {
    class_char ( cstack [j] );
  }
}

int rgx_list_dfa ( Stack * list, DState ** dfa ) {
  int nr = list->len / sizeof (void *), n, nt = 0;
  State * nfa = allocate ( sizeof (State) ),
    ** out = allocate ( (nr+1) * sizeof (State *) );
  state_reset ();
  char ** rgx = (char **) list->stack;
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
  char mem [ sizeof (void *) ];
  Stack list = (Stack) { 
    .stack = mem, .len = 0, .nentries = 0, .max = sizeof (mem) 
  };
  stack_push (&list, rgx);
  return rgx_list_dfa (&list, dfa);
}

int rgx_dfa_match ( DState * dfa, const char * txt ) {
  const char *start = txt, *end = NULL;
  DState * d = dfa;
  int c;
  if (RGXMATCH (d))  end = txt;
  while ( (c = 0xFF & *txt++) ) {
    if ( d->next[c] == NULL )
      break;
    d = d->next[c];
    if (RGXMATCH (d))  end = txt;
  }
  /* return val = number of chars that match rgx + 1 */
  return end ? (int) ( end - start + 1) : 0;
}
