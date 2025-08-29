#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "regex.h"
#include "stack.h"
#include "nfa.h"
#include "allocator.h"

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

static int state_number = 0;
static State * state ( int id, State * a, State * b ) {
  State * s = allocate ( sizeof (State) );
  s->id  = id;
  s->ist = state_number++;
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
  s->ist = state_number++;
  return s;
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

int rgx_nfa ( char * rgx, State ** start, int itoken ) {

  #define  STT(_f_,_a,_b) s = state (_f_,_a,_b); if(s == NULL) return RGXOOM
  #define  POP(_e_)       if (n) _e_ = stack[--n]; else return RGXERR;
  #define  PUSH(_s_,_d_)  if (n < RGXSIZE) stack[n++] = (Fragment){ _s_, _d_} ; \
                          else return RGXOOM

  state_number = 0;
  int * rpn = NULL, status;
  if ( (status = rgx_rpn (rgx, &rpn)) < RGXEOE ) return status;
  int n = 0, op;
  Fragment stack[RGXSIZE], e, e0, e1;
  State * s;
  while ( ( op = *rpn++ ) >= 0 ) {
    if (ISRGXOP (op)) {
      switch (op & 255) {
        case 'd' : case 's' : case 'S' :
        case 'w' : case 'D' : case 'W' :
        case '^' : case '$' : case '~' :
        case ',' : case '-' : case '#' :
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
        default:
          /* Unknown */
          return RGXERR;
      }
    }
    else {
      STT ( op, NULL, NULL );
      PUSH ( s, (Dangling *) (s->out) );
    }
  }
  POP(e);
  if (n) return RGXERR;
  concatenate ( e.out, fstate (itoken) );
  *start = e.state;
  return state_number;

  #undef  PUSH
  #undef  POP
  #undef  STT
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

int states_transition ( Stack * from, Stack * to, State *** buff, int c ) {
  stack_reset (to);
  ++counter;
  int status = 0;
  State ** stack = (State **) from->stack;
  for (int i = 0; i < from->nentries && !status; ++i ) {
    State * s = stack [i];
    if ( s->id == c )
      status = states_add ( s->out[0], to, buff );
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

int rgx_nfas ( Stack * rgx_stack, State ** nfa ) {
  int n = rgx_stack->len / sizeof (void *);
  char ** rgx = (char ** )  rgx_stack->stack;
  State * s = allocate ( sizeof (State) );
  State * child, ** out = allocate ( (n+1) * sizeof (State *) );
  for (int i=0; i<n; ++i) {
    if ( rgx_nfa (rgx[i], &out[i], i) < 0 ) return RGXERR;
  }
  *s = (State) {
    .id = NFAEPS,
    .ist = state_number++,
    .out = out
  };
  *nfa = s;
  return 0;
}
