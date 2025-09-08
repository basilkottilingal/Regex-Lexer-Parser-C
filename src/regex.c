#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "regex.h"
#include "allocator.h"

/*
.. Precedence table in decreasing order of precedence
.. ----------------|--------------
.. Operations      |    Example
.. ----------------|--------------
.. Grouping        |  (), [], [^]
.. Unary operators |  x*, x?, x+
.. Concatenation   |      x;y
.. Union           |      x|y
.. ----------------|--------------
.. Note (a) the Operator ';' is used as the hidden and implicit append
.. operator. (b) Expect a maximum depth ( for operator stack) of 2 per
.. each groupings like (), [], [^] and global grouping. Only the ()
.. can be compounded.
*/

#define  RGXCHR(_c) ( (int) ((_c) < 32 || (_c) > 126) ? EOF : (_c) )
static int rgx_input ( char ** str ) {
  /*
  .. Doesn't expect value outside [32,126) except for '\0'. Unicode
  .. characters are not allowed in the regex expr. If you want the
  .. regex to represent tokens with unicode characters, use  "\uXXXX".
  .. Non printable ascii characters, i.e c ∈ [0,32) ∪ [126, 256),
  .. are taken as end of regex expression
  */
  char c = *(*str)++;
  return RGXCHR(c);
}

static int token[3] = {0};
static int charclass = 0;

/*
.. We break down the regex to tokens of 
.. (a) literals in [0, 256) (normal ascii literals, escaped, 
.. unicode literals starting with \u, hex starting with \x).
.. (b) operators in regex : .^{}()[]<>,?+*$;,-
.. Some of those operators are defined additionally to identify some
.. operations. Ex: ';' is used for appending. ',' is also used for
.. appending, but inside character class []. '<''>' is used for 
.. negated character class [^]. 
.. (c) character groups like \D, \d, \w, \W, \s,\S.
.. Both operators and character groups are encoded by adding 256, so
.. that all number [0,256) are identified as literals and others have
.. special meaning.
.. (d) EOF : as end of expression for '\0' and other non-printable
.. ascii values outside [32,126)
*/
int rgx_token ( char ** str ) {

  #define  TOKEN()      token[0]
  #define  RTN(_c_)     return ( token[0] = (_c_) )
  #define  ERR(cond)    if (cond) return  ( token[0] = RGXERR )
  #define  INPUT()      c = rgx_input(str)
  #define  RGXNXT(_s_)  RGXCHR(*(*_s_))
  #define  HEX(_c_)     ((_c_ >= '0' && _c_ <= '9') ? (_c_ - '0') : \
                  (_c_ >= 'a' && _c_ <= 'f') ? (10 + _c_ - 'a') :   \
                  (_c_ >= 'A' && _c_ <= 'F') ? (10 + _c_ - 'A') : 16)
  int c;
  switch ( (INPUT ()) ) {
    case EOF : RTN (RGXEOE);
    case '\\' :
      switch ( (INPUT ()) ) {
        case EOF : RTN (RGXERR);
        case '0' : RTN ('\0');
        case 'a' : RTN ('\a');
        case 'f' : RTN ('\f');
        case 'n' : RTN ('\n');
        case 'r' : RTN ('\r');
        case 't' : RTN ('\t');
        case 'v' : RTN ('\v');
        case 'c' : INPUT();
          RTN ( (c <= 'Z' && c >= 'A') ? 1+c - 'A' :
                (c <= 'z' && c >= 'a') ? 1+c - 'a' : RGXERR);
        case 'd' : case 's' : case 'S' : case 'w' :
        case 'D' : case 'W' : RTN ( RGXOP(c) );
        case 'x' : /* \xHH representing [0x00,0xFF] */
          int hh = 0, d;
          for (int i=0; i<2; ++i) {
            INPUT ();
            ERR ((d = HEX(c)) == 16);
            hh = (hh << 4) | d;
          }
          RTN (hh);
        case 'p' : case 'P' : case 'b' :
          /*
          .. Not yet implemented
          */
          RTN (RGXERR);  
        case 'u' :
          /*
          .. \uHHHH represnting [0x0000-0xFFFF] split over
          .. two bytes [0x00-0xFF] [0x00-0xFF]
          */
          ERR (charclass);
          int * r = &token[1];
          for (int j=0; j<2; ++j) {
            int hh = 0;
            for (int i=0; i<2; ++i) {
              INPUT ();
              ERR ((d = HEX(c)) == 16);
              hh = (hh << 4) | d;
            }
            r[j] = hh;
          }
          RTN (RGXOP ('u'));
        default  :
          RTN (c);
      }
    case '[' :
      if (charclass)
        RTN ('[');
      if ( RGXNXT (str) == '^' ) {
        INPUT();
        charclass = 2;
        /*
        .. Replace [^..] with <..>
        */
        RTN ( RGXOP('<') );
      }
      charclass = 1;
      RTN ( RGXOP ('[') );
    case ']' :
      ERR (!charclass);
      int s = charclass;
      charclass  = 0;
      RTN ( s == 2 ? RGXOP ('>' ) : RGXOP(']'));
    case '(' :
      /*
      .. patterns ()(?...) are not yet implemented
      */
      ERR ( TOKEN() == RGXOP(')') && RGXNXT(str) == '?' );
    case ')' : case '|' : case '$' :
    case '.' : case '+' : case '*' :
      RTN ( charclass ? c : RGXOP(c) );
    case '^' :
      /*
      .. fixme : cannot handle if '^' represents start of file, but
      .. '^' is not at the beginning of rgx pattern as in the example
      ..  (\W|^)[\w.\-]{0,25}@(yahoo|hotmail|gmail)\.com(\W|$)
      */
      RTN ( TOKEN() <= 0 ? RGXOP ('^') : '^' );
    case '?' :
      RTN ( charclass ? c : RGXOP(c) );
    case '-' :
      RTN (
        ( !charclass || ( TOKEN() == RGXOP('[') ) ||
          ( RGXNXT(str) == ']' ) ) ? '-' : RGXOP ('-')
      );
    case ' ' :
      RTN ( charclass ? ' ' : RGXEOE );
    case '{' :
      int *r = &token[1], nread = 0; r[0] = 0, r[1] = INT_MAX;
      for (int i=0; i<2; ++i) {
        int ndigits = 0, val = 0;
        while ( (INPUT()) != EOF ) {
          if ( c == '}' ) {
            if (ndigits) { ++nread; r[i] = r[1] = val; }
            i=2; break;
          }
          if ( c == ',' ) {
            ERR (i);
            if (ndigits) { ++nread; r[i] = val; }
            break;
          }
          ERR ( c<'0' || c>'9' );
          ++ndigits, val = val*10 + (c-'0');
        }
      }
      /* Errors: {n,m,}, {}, {,}, {n,m} with m<n.
      .. {0}, {0,0}, {,0} will be simply omitted */
      ERR ( !nread || r[1] < r[0] );
      RTN ( !r[1] ? rgx_token (str) : RGXOP('#') );
    default :
      RTN (c);
  }
  RTN (RGXERR);

  #undef  ERR
  #undef  RTN
  #undef  INPUT
  #undef  RGXNXT
  #undef  HEX
}

enum {
  RGXBGN = 0, /* Just started parsing */
  RGXOPD = 1, /* Last action was pushing a literal operand to stack */
  RGXOPN = 2, /* Last action was pushing an operator which completes a reduction operation */
  RGXOPR = 4, /* Pushin an operator to the operators stack */
  RGXEND = 8  /* Ending */
};

typedef struct iStack {
  int * a, n, max;
} iStack;

/*
.. Reverse polish notation
*/
int rgx_rpn ( char * s, int * rpn ) {

  #define  PUSH(_s_,_c_)        ( _s_.n == _s_.max ? \
    (_s_.a[_s_.n-1] = RGXOOM) : (_s_.a[_s_.n++] = _c_))
  #define  TOP(_s_)             ( _s_.n ? _s_.a[_s_.n - 1] : '\0' )
  #define  POP(_s_)             ( _s_.n ? _s_.a[--_s_.n] : '\0' )
  #define  STACK(_a_,_m_)       (iStack) {.a = _a_, .n = 0, .max = _m_}
  #define  RTN(_s_,_c_)         if (PUSH(_s_,_c_)<RGXEOE) return TOP(_s_)
  #define  OPERATOR(_c_)        RTN(ostack,_c_); last = RGXOPR;
  #define  OPERAND(_c_)         RTN(stack,_c_);  last = RGXOPD;
  #define  OPERATION(_c_)       RTN(stack,_c_);  last = RGXOPN;

  char ** rgx = &s;
  int queued[4];
  int RGXOPS[RGXSIZE];
  iStack stack  = STACK (rpn, RGXSIZE),
        ostack = STACK (RGXOPS, RGXSIZE),
        queue  = STACK (queued, 4);
  int op, last = RGXBGN, *range = &token[1];
  while ( ( op = TOP (queue) ? POP (queue) : rgx_token (rgx) ) ) {
    if ( op == RGXEOE ) {
      for (int i=0; i<2 && TOP (ostack); ++i)
        RTN (stack, POP(ostack));
      if (TOP (ostack)) return RGXERR;
      stack.a[stack.n] = RGXEOE;
      return *rgx - s;
    }
    if ( op < 0 ) {
      stack.a[stack.n] = RGXERR;
      error ( "rgx error : wrong expression "); return op;
    }
    if ( ISRGXOP (op) ) {
      switch ( op & 255 ) {

        case 'u' :
          /* \uHHHH  split into two literals both in [0,256)*/
          OPERAND (range[0]); OPERAND (range[1]);
          break;

        case 'd' : case 's' : case 'S' :
        case 'w' : case 'D' : case 'W' :
          /* Not yet implemented */
          if ( last & (RGXOPD | RGXOPN) ) {
            RTN (queue, op);
            RTN (queue, RGXOP(charclass ? ',' : ';'));
            break;
          }
          OPERAND (op);
          break;

        /* Unary operators which are already postfix in regex. */
        case '#' :
          /* Range is also pushed
          OPERAND (range[1]); OPERAND (range[0]);
          */
        case '*' : case '?' : case '+' :
        case '~' : case '!' :
          if ( !(last & (RGXOPD | RGXOPN)) ) return RGXERR;
          OPERATION (op);
          break;

        case '^' : case '$' :
          /* Special cases. Not yet implemented*/
          break;

        /* Groupings [..], (..) and <..> (<..> is replacement for  as [^..]*/
        case '[' : case '<' : case '(' :
          if ( last & (RGXOPD | RGXOPN) ) {
            /* Handling the implicit append operator */
            RTN (queue, op);
            RTN (queue, RGXOP(';'));
            break;
          }
          OPERATOR ( op );
          break;

        case ',' : /* '-' has precedence over OR (implicit) in [] */
          if ( TOP (ostack) == RGXOP('-') ) RTN (stack, POP(ostack));
        case ';' :
          if ( TOP (ostack) == op ) RTN (stack, POP(ostack));
          OPERATOR (op);
          break;

        case '|' :
          if ( TOP (ostack) == RGXOP(';') )  RTN (stack, POP(ostack));
          if ( TOP (ostack) == RGXOP('|') )  RTN (stack, POP(ostack));
          OPERATOR (op);
          break;

        case '>' :
        case ']' :
        case ')' :
          /* For reduction of [] or [^] */
          if (charclass)
            RTN (queue, charclass == 2 ? RGXOP ('~') :  RGXOP ('!') );
          /* '>'='<'+2, ']'='['+2,  ')'='('+1 */
          int c = op - 2 + (op == RGXOP(')'));
          for (int i=0; i<2 && ( TOP (ostack) != c ); ++i)
            RTN (stack, POP(ostack));
          if ( POP (ostack) != c ) return RGXERR;
          last = RGXOPN;
          break;

        case '-' :
          /* Expects a literal before '-' operator */
          if (last != RGXOPD) return RGXERR;
          if (TOP(ostack) == op) RTN (stack, POP(ostack));
          OPERATOR (op);
          break;

        default :
          /* Unknown Operator */
          return RGXERR;
      }
    }
    else {
      if ( last & (RGXOPD | RGXOPN) ) {
        RTN (queue, op);
        RTN (queue, RGXOP(charclass ? ',' : ';'));
        continue;
      }
      OPERAND (op);
    }
  }

  return RGXERR;

  #undef  POP
  #undef  TOP
  #undef  STACK
  #undef  RTN
  #undef  OPERATOR
  #undef  OPERAND
  #undef  OPERATION
}

void rgx_free () {
  destroy ();
}

