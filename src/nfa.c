#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "regex.h"
#include "stack.h"
#include "nfa.h"
#include "allocator.h"
#include "class.h"
#include "limits.h"

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

static int   nfa_counter = 0;
static int * class = NULL;
static int   nclass = 0;

/*
.. (a) reset nfa_counter to 0
.. (b) given a list of rgx, it pre evaluate equivalence classes
*/
void nfa_reset ( char ** rgx, int nr ) {

  nfa_counter = 0;

  int csingle [256],                    /* For single char eq class */
    group [256],                      /* For charcter set [], [^..] */
    rpn [RGXSIZE],                                     /* RPN stack */
    err = 0;
  memset (csingle, 0, sizeof (csingle));

  #define ERRB(cond) if (cond) { err = 1; break; }

  /*
  .. Creating equivalence classes that accomodate all literals & also
  .. character class groups in each  pattern rgx[i]. We traverse
  .. through each of the regex and identify literals and for each
  .. group [] and [^] the partition P(Σ) is refined using
  .. class_refine (), and all the single standing literals are taken
  .. care at the end using class_char ().
  */
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
        csingle [c] = 1;           /* single char equivalence group */
        continue;
      }
      switch ( (c &= 0xFF) ) {
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
            group [ng++] = queue[--nq];
          ERRB (nq);
          class_refine (group, ng);   /* refine for character class */
          charclass = 0;
          break;
        case '.' :
          csingle ['\n'] = 1;   /* single char equivalence for '\n' */
          break;
        case 'd' : case 'D' :
        case 'w' : case 'W' :
        case 's' : case 'S' :
          ERRB (1);                      /* fixme : not implemented */
      }
    }
    ERRB (err);
  }

  for (int c=0; c<256; ++c)
    if (csingle [c])
      class_char ( c );/* refine for each single character eq class */

  if (err) {
    error ("nfa reset : unknown error in evaluating eq class");
    return;
  }

  class_get ( &class, &nclass );

  #if 0
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

/*
.. create an NFA state
*/
static State * state ( int id, State * a, State * b ) {
  State * s = allocate ( sizeof (State) );
  s->id  = id;
  s->ist = nfa_counter++;
  int nout = (a == NULL) ? 1 : 3;
  s->out = allocate ( nout * sizeof (State *) );
  if (a) {
    s->out[0] = a;
    s->out[1] = b;
  }
  return s;
}

/*
.. Create a fragment, say 'f', for a character class []. The state
.. f.state is an ε-transition NFA, which is connected to all the NFAs 
.. corresponding to each of the characters in the [] group. The "out"
.. transitions of all these NFAs are appended and stored in f.out
*/
static Fragment state_class ( int * classes, int cmp ) {
  int stack [256], n = 0;
  for (int i=0; i<nclass; ++i)
    if (classes[i] == cmp)
      stack [n++] = i;

  if (!n) 
    return (Fragment) {NULL, NULL};

  State * sptr = allocate ((n+1) * sizeof (State)), *s = sptr;
  State ** outptr = allocate ((2*n+1) * sizeof (State *)),
    ** out = outptr;
  
  Dangling * d = (Dangling *) outptr;
 
  for (int i=0; i<n; ++i, ++s, ++out, d++) {
    s->out = out;
    s->ist = nfa_counter++;
    s->id = stack[i];
    out [n] = s;
    d->next = d+1;          /* append all the dangling out pointers */
  }
  d[-1].next = NULL;

  /*
  .. Put all the possible class transitions as output(s) of an ε-NFA
  */
  s->ist = nfa_counter++;
  s->id  = NFAEPS;
  s->out = out;
  out [n] = NULL;

  return (Fragment) {s, (Dangling *) outptr};
}

/*
.. An accepting NFA state
*/
struct fState {
  State s;
  int itoken;
};

/*
.. create an accpeting nfa
*/
static State * fstate ( int itoken ) {
  struct fState * f = allocate ( sizeof (struct fState) );
  f->itoken = itoken;
  State * s = &(f->s);
  s->id  = NFAACC;
  s->ist = nfa_counter++;
  return s;
}

/*
.. Get the token id in [1, tokenmax] for an accpeting state.
.. Note : id "0" should be reserved for error/reject.
*/
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

static int rpn_backtrack (int * rpn, int qpos) {
  int stack[RGXSIZE], depth = 0, c, lookfor;
  stack[depth++] = 1;
  do {
    if ( !qpos ) return RGXERR;
    if ( (c = rpn [--qpos]) <= 0xFF ) {
      while (depth && --stack[depth-1] == 0)  
        depth--;      /* character or character group. operand. POP */
      continue;
    }
   
    switch ( (c &= 0xFF) ) { 
      case ']' : case '>' :
        lookfor = RGXOP (c - 2);
        do {
          if (!qpos) return RGXERR;
          c = rpn [--qpos];
        } while (c != lookfor);
      case '.' : case 's' : case 'S' : case 'w' :
      case 'W' : case 'd' : case 'D' : 
        while (depth && --stack[depth-1] == 0)  
          depth--;      /* character or character group. operand. POP */
        break;

      case '|' : case ';' :                 /* binary operator. PUSH */
      stack [depth++] = 2;
      break;

      case '}' :
        lookfor = RGXOP ('q');
        do {
          if (!qpos) return RGXERR;
          c = rpn [--qpos];
        } while (c != lookfor);
      case '+' : case '*' : case '?' :
        stack [depth++] = 1;          
        break;                                     /* unary op. PUSH */
      default :
        return RGXERR;
    }

  } while ( depth );

  return qpos;
}

typedef struct Quantifier {
  int backtrack, id, iter;
  char * op;
  struct Quantifier * next;
} Quantifier;

Quantifier * quantifier (Quantifier ** root, int * rpn, int irpn) {
  #define PUSH(_c_) if (len == RGXSIZE) return NULL;                 \
    op[len++] = _c_
  Quantifier * Q;
  while ( (Q = *root) != NULL ) {
    if (Q->id == irpn) return Q;
    root = & Q->next;
  }
  int b = rpn_backtrack (rpn, irpn),
    m = rpn [irpn + 2], n = rpn [irpn + 3],
    first = 1, len = 0;
  char op [RGXSIZE];
  if (b == RGXERR) return NULL;  
  for (int i=0; i<m; ++i) {
    PUSH ('x');
    if (first) first = 0;
    else { PUSH (';'); }
  }
  if ( n == INT_MAX ) {
    PUSH ('x'); PUSH ('*');
    if (first) first = 0;
    else { PUSH (';'); }
  }
  else {
    for (int i=0; i<n-m; ++i) {
      PUSH ('x'); PUSH ('?');
      if (first) first = 0;
      else { PUSH (';'); }
    }
  }
  PUSH ('\0');
  *root = Q = allocate (sizeof (Quantifier));
  *Q = (Quantifier) {
    .backtrack = b,
    .id = irpn,
    .op = allocate_str (op),
    .iter = 1
  };
  #if 0
  printf ("\n%s where x is (",op);
  for(int i=b; i<irpn; ++i)
    printf ("%c", rpn[i] & 0xFF);
  printf (")");
  #endif
  return Q;
  #undef PUSH
}

/*
.. Create NFA tree for RPN corresponding to a regex pattern
*/
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
  #define  UNQUEUE(_c_)     queue[--nq]
  #define  GROUP(_c_)       classes [class[_c_]] = 1

  int nnfa = nfa_counter;
  int n = 0, op, charclass = 0, classes [256], queue [4], nq,
    quant = 0;
  Quantifier * root = NULL;
  Fragment stack[RGXSIZE], e, e0, e1;
  State * s;

  int irpn = 0;
  while ( ( op = quant ? quant : rpn[irpn++] ) >= 0 ) {
    quant = 0;
    if (ISRGXOP (op)) {
      switch ( (op &= 0XFF) ) {
        case 'd' : case 's' : case 'S' :
        case 'w' : case 'D' : case 'W' :
        case '^' : case '$' :
          /* Not yet implemented */
          return RGXERR;
        case 'q' :
          /*
          .. used for x{m,n} quantifier. we backtrack looking the node
          .. on which the quantifying happens. so we have to backtrack
          .. the rpn until we find the subset of rpn on which {m,n}
          .. operates.
          */
          /*
          .. q should be followed by {m,n}. An (x){m,n} quantifier can
          .. be replaced by
          .. ( (x) (x) (x) ...m times ) ( (x)? (x) ? ... n-m times )
          .. Similarly an (x){m,} can be replaced by
          .. ( (x) (x) (x) ...m times ) (x)*
          .. What we do is simply, replace 'q' by ';' (append)
          .. and go back the rpn to the appropriate location and
          .. iterate again until {m,n} is reduced to {0, n-m} or
          .. {0, infty}. In first case we reiterate with '?' 
          .. followed by ';' (n-m times). In second case we
          .. reiterate with '*'.
          */
          Quantifier * Q = quantifier (&root, rpn, irpn-1);
          if (Q == NULL) return RGXERR;
          char q = Q->op[Q->iter++];
          switch ( q ) {
            case 'x' :
              irpn = Q->backtrack;    /* Traverse back */
              break;
            case '\0' :
              Q->iter = 1;
              irpn += 4;
              break;
            default :
              quant = RGXOP(q);   /* add '?'/'*'/';' to the queue */
              irpn --;
          }
          break;
        case ';' :
          POP (e1); POP (e0);
          concatenate ( e0.out, e1.state );
          PUSH ( e0.state, e1.out );
          break;
        case '|' :
          POP (e1); POP (e0);
          STT ( NFAEPS, e0.state, e1.state );
          append ( e0.out, e1.out );
          PUSH ( s, e0.out );
          break;
        case '+' :
          POP (e);
          STT ( NFAEPS, e.state, NULL );
          concatenate ( e.out, s );
          PUSH ( e.state, (Dangling *) (& s->out[1]) );
          break;
        case '*' :
          POP (e);
          STT ( NFAEPS, e.state, NULL );
          concatenate ( e.out, s );
          PUSH ( s, (Dangling *) (& s->out[1]) );
          break;
        case '?' :
          POP (e);
          STT ( NFAEPS, e.state, NULL );
          PUSH ( s, append (e.out, (Dangling *) (& s->out[1]) ) );
          break;
        case '[' :
        case '<' :
          charclass = 1;
          memset (classes, 0, nclass * sizeof (int));
          nq = 0;
          break;
        case '>' :
        case ']' :
          if (nq) GROUP (UNQUEUE ());
          if (nq) return RGXERR;
          charclass = 0;
          e = state_class (classes, (op == '>') ? 0 : 1 );
          if (e.state == NULL)          /* Empty character class [] */
            return RGXERR;
          PUSH (e.state, e.out);
          break;
        case ',' :
          while (nq) 
            GROUP ( UNQUEUE () );
          break;
        case '-' :
          if ( nq < 2 ) return RGXERR;
          int b = UNQUEUE (), a = UNQUEUE ();
          for (int k=a; k<=b; ++k)
            GROUP ( k );
          break;
        case '.' :
          memset (classes, 0, nclass * sizeof (int));
          classes [class ['\n']] = 1;  /* any character except '\n' */
          e = state_class (classes, 0); 
          PUSH (e.state, e.out);
          break;
        default:
          error ("rgx nfa : unimplemented rule");        /* Unknown */
          return RGXERR;
      }
    }
    else {
      if ( charclass ) {
        QUEUE ( op );
        continue;
      }
      STT ( class [op], NULL, NULL );
      PUSH ( s, (Dangling *) (s->out) );
    }
  }

  if ( op < EOF || n != 1 ) {
    error ("rpn nfa : wrong regex pattern ");
    return RGXERR;
  }

  POP (e);
  concatenate ( e.out, fstate (itoken) );
  *start = e.state;
  return nfa_counter - nnfa;        /* Return number of nfa created */

  #undef  PUSH
  #undef  POP
  #undef  STT
  #undef  QUEUE
  #undef  UNQUEUE
  #undef  GROUP
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
states_transition ( Stack * from, Stack * to,
    State *** buff, int ec )
{
  stack_reset (to);
  ++counter;
  int status = 0;
  State ** stack = (State **) from->stack;
  for (int i = 0; i < from->nentries && !status; ++i ) {
    State * s = stack [i];
    if ( s->id == ec )
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
    status = states_transition ( s0, s1, buff, class [c] );
    t = s0; s0 = s1; s1 = t;
    if ( status )         {  RTN (status); }
    if ( RGXMATCH (s0) )  {  end = txt; continue; }
    if ( !s0->nentries )     break;
  }
  /* return val = number of chars that match rgx + 1 */
  RTN (end ? (int) (end - start + 1) : 0);

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
  nfa_reset (&rgx, 1);
  State * nfa = NULL;
  if ( rgx_nfa (rgx, &nfa, 0) < 0 ) {
    error ("rgx match : regex error");
    return RGXERR;
  }
  /* fixme :  recollect the used memory */
  return rgx_nfa_match (nfa, txt);
}
