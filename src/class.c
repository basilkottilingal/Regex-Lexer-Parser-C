/*
.. Given an NFA (Q, Σ, δ, q0 , F ) there exists
.. atleast one partition P (Σ), which satisfy the condition ( cond-1 )
..   ∀ p[i] ∈ P and c, c′ ∈ p[i]   ⇒   δ(q, c) = δ(q, c′)  ∀  q ∈ Q
.. Now, the equivalence class is the function
..   C(c) : Σ → {0, 1, ..., |P| − 1} which maps c to p[i]
.. i.e.
..   C(c) = i ⇐⇒ c ∈ p[i]
.. Our aim is to look for the coarsest partition, or the one
.. that satisfy above condition (cond-1) and minimises |P(Σ)|.
..
.. We initialize ( class_init() )  with
.. P ← {Σ}
.. for each character group S, we refine P ( class_refine() ) as :
..    P ← { Y ∩ S, Y ∖ S ∣ X ∈ P } ∖ { ∅ }
..
.. In the above algorithm, S, is a character set
.. like [a-z], [^0-9], etc.
.. Neglect intersection set or difference set, if empty.
*/

#define ALPH 256
#define END  -1

#include <string.h>
#include <assert.h>

#include "class.h"

/*
.. head and next are used to maintain a linked list of characters in
.. an equivalence class. END is used to mark the end of the list
*/
static int next [ALPH], head [ALPH];

/*
.. class is the mapping C(c) from [0,256) -> [0, nclass).
.. nclass is the number of equivalence classes.
.. nclass = max(C) + 1 = |P|
*/
static int class [ALPH], nclass = 0;

void class_get ( int ** c, int * n ) {
  *c = class; *n = nclass;
}

void class_init () {

  /*
  .. Initialize with just one equivalence class : P ← {Σ}
  .. and C(c) = 0 ∀ c in Σ
  */

  nclass = 1;
  memset (class, 0, sizeof (class));
  head [0] = 0;
  for(int i=0; i<ALPH-1; ++i)
    next [i] = i+1;
  next [ALPH-1] = END;

}


void class_refine ( int * list, int nlist ) {
  #define INSERT(Q,q)          *Q = q; Q = & next [q]

  /*
  ..  if (nclass == ALPH)       already refined to max. i.e |Σ| = |P|
  ..    return;
  */

  int S [ALPH], * Y1, * Y2, c, n, ec;
  memset (S, 0, sizeof (S));
  for (int i=0; i<nlist; ++i)
    S [list[i]] = 1;

  for (int i=0; i<nlist; ++i) {

    c = list [i];

    if ( S[c] == 0 )                /* Alreay placed to a new class */
      continue ;

    ec = class [c];
    Y1 = & head [ec] ;
    Y2 = & head [nclass] ;

    c = *Y1;
    while ( c != END ) {
      n = next [c];
      if ( S [c] ) {
        INSERT (Y1, c);                               /* Y1 = Y ∩ S */
      }
      else {
        S [c] = 0;
        INSERT (Y2, c);                               /* Y2 = Y ∖ S */
      }
      c = n;
    }
    *Y1 = *Y2 = END;                     /* Now Y is replaced by Y1 */

    assert ( head [ec] != END );                /* Y1 can't be empty*/
    if ( (c = head [nclass]) != END ) {              /* Y ∖ S  ≠ ∅  */
      //c = head [ec]; 
      do {
        class [c] = nclass;
        c = next [c];
      } while ( c != END ) ;
      nclass++;                        /* Add Y2 to P, if non-empty */
    }
  }

  #undef INSERT
}

#undef END


