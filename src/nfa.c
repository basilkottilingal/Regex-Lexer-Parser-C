#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "regex.h"
#include "stack.h"
#include "nfa.h"
#include "allocator.h"
#include "class.h"

/*
.. Functions and objects required for creating a
.. NFA ( Non-deterministic Finite Automaton )
*/

/*
.. Maintain a linked list of dangling transitions
*/
typedef struct Dangling {
  struct Dangling * next;
} Dangling;

/*
.. An NFA Fragment which has a starting state "s"  and
.. a linked list of some non-connected (dangling) edges
*/
typedef struct Fragment {
  State * state;
  Dangling * out;
} Fragment;

static int nfa_counter = 0;
static int * class = NULL;
static int nclass = 0;

/*
.. (a) reset nfa_counter to 0
.. (b) given a list of rgx, it pre evaluate equivalence classes
*/
void nfa_reset ( Stack * rgxlist ) {

  nfa_counter = 0;

  int csingle [256], cstack [256],      /* For single char eq class */
    nc = 0, group [256],              /* For charcter set [], [^..] */
    nr = rgxlist->len / sizeof (void *),  /* number of rgx patterns */
    rpn [RGXSIZE], err = 0;

  #define ERRB(cond) if (cond) { err = 1; break; }

  char ** rgx = (char **) rgxlist->stack;
  memset (csingle, 0, sizeof (csingle));

  class_init ();

  for (int i=0; i<nr; ++i) {
    rgx_rpn (rgx[i], rpn);
    int j = 0, c, ng = 0, queue [4], nq = 0, charclass = 0;
    while ( (c=rpn[j++]) >= 0 ) {
      ERRB (err || nq == 4 || ng == 256);
      if (!ISRGXOP (c)) {
        if (charclass) {
          queue [nq++] = c;
          continue;
        }
        if (csingle [c])
          continue;
        csingle [c] = 1;           /* single char equivalence group */
        cstack [nc++] = c;
        continue;
      }
      switch ( (c &= 255) ) {
        case '[' :  case '<' :
          ng = 0, charclass = 1;
          break;
        case '-' :
          ERRB (nq < 2);  /* need two operand to calculate range a-b*/
          int b = queue [--nq], a = queue[--nq];
          for (int k=a; k<=b && ng < 256; ++k)
            group [ng++] = k;
          break;
        case ',' :
          ERRB (nq > 2 || ng > 254);
          while (nq)
            group [ng++] = queue[--nq];
          break;
        case ']' : case '>' :
          if (nq)
            group [ng++] = c;
          ERRB (nq);
          class_refine (group, ng);  /* refine for character class */
          charclass = 0;
          break;
        case 'd' : case 'w' : case 's' :
        case 'D' : case 'W' : case 'S' :
        case '.' :
          ERRB (1); //fixme : implement
      }
    }
    ERRB (err);
  }

  while (nc--) {
    class_char ( cstack [nc] );
  }

  if (err) {
    error ("nfa reset : unknown error in evaluating eq class");
    return;
  }

  class_get ( &class, &nclass );
  #if 0
  int * class = NULL, nclass = 0;
  
  char buff[] = " ";
  printf ("\n [  ");
  for (int i=0; i<256; ++i) {
    buff [0] = (char) i;
    printf ("%s %d  ", i>32 && i<126 ? buff : " ", class[i]);
    if ( (i & (int) 15) == 0)
      printf ("\n    ");
  }
  printf ("\n ]");
  #endif

  #undef ERRB
}

static State * state ( int id, State * a, State * b ) {
  State * s = allocate ( sizeof (State) );
  s->id  = id;
  s->ist = nfa_counter++;
  int nout = (a == NULL) ? 1 : 3;
  s->out = allocate ( nout * sizeof (State *) );
  s->out[0] = a;
  if (a) s->out[1] = b;
  return s;
}

struct fState {
  State s;
  int itoken;
};

static State * fstate ( int itoken ) {
  struct fState * f = allocate ( sizeof (struct fState) );
  f->itoken = itoken;
  State * s = &(f->s);
  s->id  = NFAACC;
  s->ist = nfa_counter++;
  return s;
}

int state_token ( State * f ) {
  assert (f->id == NFAACC);
  return ((struct fState*) f)->itoken;
}

static Dangling * append ( Dangling * d, Dangling * e ) {
  Dangling * old = d;
  while (d->next) {
    d = d->next;
  }
  d->next = e;
  return old;
}

static void concatenate ( Dangling * d, State * s ) {
  Dangling * next;
  while (d) {
    next = d->next;
    *((State **) d) = s;
    d = next;
  }
}

int rpn_nfa ( int * rpn, State ** start, int itoken ) {

  #define  STT(_f_,_a,_b)   s = state (_f_,_a,_b);                   \
                            if(s == NULL) return RGXOOM
  #define  POP(_e_)         if (n) _e_ = stack[--n];                 \
                            else return RGXERR;
  #define  PUSH(_s_,_d_)    if (n < RGXSIZE)                         \
                              stack[n++] = (Fragment){ _s_, _d_} ;   \
                            else return RGXOOM
  #define  QUEUE(_c_)       if (nq == 4) return RGXOOM;              \
                            queue[nq++] = _c_
  #define  UNQUEUE(_c_)     ( nq ? queue[--nq] : EOF )
  #define  CLASS(_c_)                                                \
     ( nchar < 256 ? (charstack[nchar++] =  _c_) : RGXOOM )

  int nnfa = nfa_counter;
  int n = 0, op, charclass = 0, charstack [256], nchar, queue [4], nq;
  Fragment stack[RGXSIZE], e, e0, e1;
  State * s;
  while ( ( op = *rpn++ ) >= 0 ) {
    if (ISRGXOP (op)) {
      switch (op & 255) {
        case 'd' : case 's' : case 'S' :
        case 'w' : case 'D' : case 'W' :
        case '^' : case '$' : case '{' :
        case '.' :
          /* Not yet implemented */
          return RGXERR;
        case ';' :
          POP(e1); POP(e0);
          concatenate ( e0.out, e1.state );
          PUSH ( e0.state, e1.out );
          break;
        case '|' :
          POP(e1); POP(e0);
          STT ( NFAEPS, e0.state, e1.state );
          append ( e0.out, e1.out );
          PUSH ( s, e0.out );
          break;
        case '+' :
          POP(e);
          STT (NFAEPS, e.state, NULL );
          concatenate ( e.out, s );
          PUSH ( e.state, (Dangling *) (& s->out[1]) );
          break;
        case '*' :
          POP(e);
          STT (NFAEPS, e.state, NULL );
          concatenate ( e.out, s );
          PUSH ( s, (Dangling *) (& s->out[1]) );
          break;
        case '?' :
          POP(e);
          STT (NFAEPS, e.state, NULL );
          PUSH ( s, append (e.out, (Dangling *) (& s->out[1]) ) );
          break;
        case '[' :
        case '<' :
          charclass = 1;
          nq = nchar = 0;
          break;
        case ']' :
          if (nq) { CLASS (UNQUEUE ()); }
          if (nq) return RGXERR;
          charclass = 0;
          break;
        case '>' :
          break;
        case ',' :
          while (nq) {
            CLASS ( UNQUEUE () );
          }
          break;
        case '-' :
          int b = UNQUEUE (), a = UNQUEUE ();
          if ( a == EOF || b == EOF ) return RGXERR;
          for (int k=a; k<=b; ++k) {
            CLASS ( k );
          }
          break;
        default:
          /* Unknown */
          error ("rgx nfa : unimplemented rule");
          return RGXERR;
      }
    }
    else {
      if ( charclass ) {
        QUEUE ( op );
        continue;
      }
      STT ( op, NULL, NULL );
      PUSH ( s, (Dangling *) (s->out) );
    }
  }
  POP(e);
  if (n) {
    error ("rpn nfa : wrong rpn");
    return RGXERR;
  }
  concatenate ( e.out, fstate (itoken) );
  *start = e.state;
  return nfa_counter - nnfa;           /* Return num of nfa created */

  #undef  PUSH
  #undef  POP
  #undef  STT
  #undef  QUEUE
  #undef  UNQUEUE
  #undef  CLASS
}

int rgx_nfa ( char * rgx, State ** start, int itoken ) {
  int rpn [RGXSIZE];
  if ( rgx_rpn (rgx, rpn) < RGXEOE ) {
    error ("rgx nfa : cannot make rpn for rgx \"%s\"", rgx);
    return RGXERR;
  }
  return rpn_nfa ( rpn, start, itoken );
}

/*
.. Add all the states including "start" to the "list", that
.. can be attained from "start" with epsilon transition.
*/
static int counter = 0;
int states_add ( State * start, Stack * list, State *** stack) {
  /*
  .. We use the "buff" stack when we go down the nfa tree
  .. and thus avoid recusrive call
  */
  State * s; int n = 0;
  stack[n++] = ( State * [] ) {start, NULL};
  while ( n ) {
    /* Go down the tree, if the State is an "NFAEPS" i.e epsilon */
    while ( (s = *stack[n-1]) ) {
      if ( s->id != NFAEPS || s->counter == counter ) break;
      /* PUSH() to the stack */
      if ( n < RGXSIZE ) stack[n++] = s->out;
      else return RGXOOM;
    }

    /* POP() from the stack. Add the state to the list */
    do {
      s = *stack[n-1]++;
      s->counter = counter;
      stack_push ( list, s );
      if (s->id == NFAACC) RGXMATCH(list) = 1;
    } while ( *stack[n-1] == NULL && --n );
  }

  return 0;
}

int states_at_start ( State * nfa, Stack * list, State *** buff ) {
  stack_reset (list);
  ++counter;
  return states_add ( nfa, list, buff );
}

int
states_transition ( Stack * from, Stack * to, State *** buf, int c ) {
  stack_reset (to);
  ++counter;
  int status = 0;
  State ** stack = (State **) from->stack;
  for (int i = 0; i < from->nentries && !status; ++i ) {
    State * s = stack [i];
    if ( s->id == c )
      status = states_add ( s->out[0], to, buf );
  }
  return status;
}

int rgx_nfa_match ( State * nfa, const char * txt ) {

  #define RTN(r)    stack_free(s0); stack_free(s1); return r

  State ** buff[RGXSIZE];
  Stack * s0 = stack_new (0), * s1 = stack_new(0), * t;
  const char * start = txt, * end = NULL;
  int status = states_at_start ( nfa, s0, buff ), c;
  if (status)         { RTN (status); }
  if (RGXMATCH (s0) )   end = txt;
  while ( (c =  0xFF & *txt++) ) {
    status = states_transition ( s0, s1, buff, c );
    t = s0; s0 = s1; s1 = t;
    if ( status )         {  RTN (status); }
    if ( RGXMATCH (s0) )  {  end = txt; continue; }
    if ( !s0->nentries )     break;
  }
  /* return val = number of chars that match rgx + 1 */
  RTN (end ? (int) ( end - start + 1) : 0);

  #undef RTN
}

int states_bstack ( Stack * list, Stack * bits ) {
  State ** s = (State ** ) list->stack;
  stack_clear (bits);
  int n = list->nentries;
  while (n--) {
    stack_bit ( bits, s[n]->ist );
  }
  return 1;
}

int rgx_match ( char * rgx, const char * txt ) {
  State * nfa = NULL;
  char mem [ sizeof (void *) ];
  Stack list = (Stack) {
    .stack = mem, .len = 0, .nentries = 0, .max = sizeof (mem)
  };
  nfa_reset (&list);
  if ( rgx_nfa (rgx, &nfa, 0) < 0 ) {
    error ("rgx match : regex error");
    return RGXERR;
  }
  /* fixme :  recollect the used memory */
  return rgx_nfa_match (nfa, txt);
}
