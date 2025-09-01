#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "regex.h"
#include "allocator.h"

/*
..Precedence table
..
.. Operations      --|--    Example    --|--  Precedence
.. Grouping          |    (), [], [^]    |    - ( Immediately push to stack )
.. Unary operators   |    x*, x?, x+     |    - ( Immediately push to stack )
.. Concatenation     |        x;y        |    1 ( Highest )  **
.. Union             |        x|y        |    2 ( Lowest )
..
..    NOTE : Operator ';' as append (implicit)
..
.. Expect a maximum depth ( for operator stack) of 2 per each groupings
.. like (...), [...], [^...] and global grouping. Only the (..) can be compunded.
*/

#define  RGXCHR(_c_)     ((int) ((_c_) < 32 ? '\0' : (_c_)))
static int rgx_input ( char ** str ) {
  /*
  .. Doesn't expect value outside [32,128) except for '\0'.
  .. Unicode characters not allowed in regex expression.
  .. If you want the regex to represent tokens with
  .. unicode characters, use "\uXXXX".
  .. Non printable characters ASCII [0,32)
  .. are taken as end of regex expression
  */
  char c = *(*str)++;
  return RGXCHR(c);
}

static int token[3] = {0};
static int single_char = 0;

int rgx_token ( char ** str ) {

  #define  TOKEN()         token[0]
  #define  RTN(_c_)        return ( token[0] = (_c_) )
  #define  INPUT()         c = rgx_input(str)
  #define  RGXNXT(_s_)     RGXCHR(*(*_s_))
  #define  HEX(_c_)        ((_c_ >= '0' && _c_ <= '9') ? (_c_ - '0') :\
                            (_c_ >= 'a' && _c_ <= 'f') ? (10 + _c_ - 'a') :\
                            (_c_ >= 'A' && _c_ <= 'F') ? (10 + _c_ - 'A') : 16)
  int c;
  switch ( (INPUT()) ) {
    case '\0' : RTN (RGXEOE);
    case '\\' :
      switch ( (INPUT()) ) {
        case '\0': RTN (RGXERR);
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
            INPUT();
            if ( (d = HEX(c)) == 16) RTN (RGXERR);
            hh = (hh << 4) | d;
          }
          RTN (hh);
        case 'p' : case 'P' : case 'b' :
          RTN (RGXERR);  /* Not yet implemented */
        case 'u' :
          /* \uHHHH represnting [0x0000-0xFFFF] split over
          .. two bytes [0x00-0xFF] [0x00-0xFF] */
          if (single_char) RTN(RGXERR);
          int * r = &token[1];
          for (int j=0; j<2; ++j) {
            int hh = 0;
            for (int i=0; i<2; ++i) {
              INPUT();
              if ( (d = HEX(c)) == 16) RTN (RGXERR);
              hh = (hh << 4) | d;
            }
            r[j] = hh;
          }
          RTN (RGXOP ('u'));
        default  :
          RTN (c);
      }
    case '[' :
      if (single_char)
        RTN ('[');
      if ( RGXNXT (str) == '^' ) {
        INPUT();
        single_char = 2;
        RTN ( RGXOP('<') );
        /* Negation. i.e, single character except those listed in [^..] */
      }
      single_char = 1;
      RTN ( RGXOP ('[') );
    case ']' :
      if (!single_char)
        RTN (RGXERR);
      int s = single_char;
      single_char  = 0;
      RTN ( s == 2 ? RGXOP ('>' ) : RGXOP(']'));
    case '(' :
      if ( TOKEN() == RGXOP(')') && RGXNXT(str) == '?' )
        RTN (RGXERR);  /* patterns ()(?...) are not yet implemented */
    case ')' : case '|' : case '$' :
    case '.' : case '+' : case '*' :
      RTN ( single_char ? c : RGXOP(c) );
    case '^' :
      /*
      .. !! Cannot handle (\W|^)[\w.\-]{0,25}@(yahoo|hotmail|gmail)\.com(\W|$)
      .. '^' at the beginning (start anchor) is defined as operator ^.
      .. (negated single character )[^..] is replaced by  <..>.
      .. Otherwise '^' is taken as literal.
      */
      RTN ( TOKEN() <= 0 ? RGXOP ('^') : '^' );
    case '?' :
      RTN ( single_char ? c : RGXOP(c) );
    case '-' :
      RTN (
        ( !single_char || ( TOKEN() == RGXOP('[') ) || ( RGXNXT(str) == ']' ) ) ?
        '-' : RGXOP ('-')
      );
    case ' ' :
      RTN ( single_char ? ' ' : RGXEOE );
    case '{' :
      int *r = &token[1], nread = 0; r[0] = 0, r[1] = INT_MAX;
      for (int i=0, ndigits = 0, val = 0; i<2; ++i, ndigits = 0, val = 0) {
        while ( (INPUT()) != '\0' ) {
          if ( c == '}' ) {
            if (ndigits) { ++nread; r[i] = r[1] = val; }
            i=2; break;
          }
          if ( c == ',' ) {
            if (i) RTN (RGXERR);
            if (ndigits) { ++nread; r[i] = val; }
            break;
          }
          if ( c<'0' || c>'9' ) RTN (RGXERR);
          ++ndigits, val = val*10 + (c-'0');
        }
      }
      /* Errors: {n,m,}, {}, {,}, {n,m} with m<n.
      .. {0}, {0,0}, {,0} will be simply omitted */
      RTN ( (!nread || r[1] < r[0]) ? RGXERR : !r[1] ? rgx_token (str) : RGXOP('#') );
    default :
      RTN (c);
  }
  RTN (RGXERR);

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
int rgx_rpn ( char * s, int ** rpn ) {

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
  int RGXRPN[RGXSIZE];
  int RGXOPS[RGXSIZE];
  iStack stack  = STACK (RGXRPN, RGXSIZE),
        ostack = STACK (RGXOPS, RGXSIZE),
        queue  = STACK (queued, 4);
  int op, last = RGXBGN, *range = &token[1];
  while ( ( op = TOP (queue) ? POP (queue) : rgx_token (rgx) ) ) {
    if ( op == RGXEOE ) {
      int depth = 0;
      for (int i=0; i<2 && TOP (ostack); ++i)
        RTN (stack, POP(ostack));
      if (TOP (ostack)) return RGXERR;
      stack.a[stack.n] = RGXEOE;
      *rpn = allocate ( (stack.n+1) * sizeof (int) );
      memcpy ( *rpn, RGXRPN,  (stack.n+1) * sizeof (int) );
      return RGXEOE;
    }
    if ( op <= 0 ) {
      stack.a[stack.n] = RGXERR;
      *rpn = allocate ( (stack.n+1) * sizeof (int) );
      memcpy ( *rpn, RGXRPN,  (stack.n+1) * sizeof (int) );
      printf("\nRgx Error : Wrong Expression "); return op;
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
            RTN (queue, RGXOP(single_char ? ',' : ';'));
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
          if (single_char)
            RTN (queue, single_char == 2 ? RGXOP ('~') :  RGXOP ('!') );
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
        RTN (queue, RGXOP(single_char ? ',' : ';'));
        continue;
      }
      OPERAND (op);
    }
  }
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

