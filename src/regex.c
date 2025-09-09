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
.. Unary operators |  x*, x?, x+, x{m}
.. Concatenation   |      x;y
.. Union           |      x|y
.. ----------------|--------------
.. Note (a) the Operator ';' is used as the hidden and implicit append
.. operator. (b) Expect a maximum depth ( for operator stack) of 2 per
.. each groupings like (), [], [^] and global grouping. Only the ()
.. can be compounded.
*/

#define  RGXCHR(_c) ( ((_c) < 32 || (_c) > 126) ? EOF : (int) (_c) )
static int rgx_input ( char ** str ) {
  /*
  .. Doesn't expect value outside [32,126) except for '\0'. Unicode
  .. characters are not allowed in the regex expr. If you want the
  .. regex to represent tokens with unicode characters, use  "\uXXXX".
  .. Non printable ascii characters, i.e c ∈ [0,32) ∪ (126, 256),
  .. are taken as end of regex expression
  */
  char c = *(*str)++;
  return RGXCHR(c);
}

static int token[3] = {0};
static int charclass = 0;

/*
.. We break down the regex to tokens of
.. (a) literals in [0, 0xFF) (normal ascii literals, escaped
.. literals, unicode literals starting with \u, hex starting with \x).
.. Note that, unicode literal > 0xFF has to be split to multiple
.. literals each in [0, 0xFF) and appended one after the other.
.. (b) operators in regex : ^{}()[]<>?+*$;,-
.. Some of those operators are defined and used temporarily to
.. identify some operations. Ex: ';' is used for appending.
.. ',' is also used for appending, but inside character class [].
.. '<' and '>' is used for the negated character class [^].
.. (c) character groups like ., \D, \d, \w, \W, \s and \S.
.. Both operators and character groups are encoded by adding 256, so
.. that all number [0,256) are identified as literals and others have
.. special meaning.
.. (d) EOF : as end of expression for '\0' and other non-printable
.. ascii values outside [32,126]
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
        case 'p' : case 'P' : case 'b' : case 'u' : case 'U' :
          error ("rgx : not yet implemented \\%c", (char) c);
          RTN (RGXERR);  
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
      RTN ( !r[1] ? rgx_token (str) : RGXOP('{') );
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
  RGXBGN = 0, /* Just started */
  RGXEND = 8, /* Ending */
              /* Last action was pushing .. */
  RGXOPD = 1, /* .. a literal operand to stack */
  RGXOPN = 2, /* .. an operator to ostack i.e reduction operation */
  RGXOPR = 4  /* .. an operator to the operators stack */
};

typedef struct iStack {
  int * a, n, max;
} iStack;

/*
.. Reverse polish notation
*/

int rgx_rpn ( char * s, int * rpn ) {

  #define  TOP(_s_)           ( _s_.n ? _s_.a[_s_.n - 1] : 0 )
  #define  POP(_s_)           ( _s_.n ? _s_.a[--_s_.n] : RGXOOM )
  #define  STACK(_a_,_m_)     (iStack) {.a = _a_, .n = 0, .max = _m_}
  #define  PUSH(_s_,_c_)      if (( _s_.n == _s_.max ?               \
    (_s_.a[_s_.n-1] = RGXOOM) : (_s_.a[_s_.n++] = _c_)) < EOF )   \
                                 return TOP(_s_)
  #define  OPERATOR(_c_)      PUSH (ostack, _c_); last = RGXOPR;
  #define  OPERAND(_c_)       PUSH (stack,  _c_); last = RGXOPD;
  #define  OPERATION(_c_)     PUSH (stack,  _c_); last = RGXOPN;

  char ** rgx = &s;
  int queued[4], RGXOPS[RGXSIZE], op, last = RGXBGN;
  iStack ostack = STACK (RGXOPS, RGXSIZE),
    stack = STACK (rpn, RGXSIZE),
    queue = STACK (queued, 4);
  while ( 1 ) {
    op = queue.n ? queue.a[--queue.n] : rgx_token (rgx) ;
    if ( op == RGXEOE ) {
      for (int i=0; i<2 && TOP (ostack); ++i)
        PUSH (stack, POP(ostack));
      if (TOP (ostack)) return RGXERR;
      stack.a[stack.n] = RGXEOE;
      return *rgx - s;
    }
    if ( op < 0 ) {
      stack.a[stack.n] = RGXERR;
      error ("rgx error : wrong expression "); 
      return op;
    }
    if ( ISRGXOP (op) ) {
      switch ( op & 255 ) {
        /*
        .. Character groups . Not yet implemented
        */
        case 'd' : case 's' : case 'w' :
        case 'D' : case 'S' : case 'W' :
        case '.' :
          if ( last & (RGXOPD | RGXOPN) ) {
            PUSH (queue, op);
            PUSH (queue, RGXOP(charclass ? ',' : ';'));
            break;
          }
          OPERAND (op);
          break;

        /* 
        .. Unary operators which are already postfix in regex.
        */
        case '*' : case '?' : case '+' :
        case '~' : case '!' :
          if ( !(last & (RGXOPD | RGXOPN)) ) return RGXERR;
          OPERATION (op);
          break;
        case '{' :
          if ( !(last & (RGXOPD | RGXOPN)) ) return RGXERR;
          int n = token [1], m = token [2];
          OPERATION (op); OPERATION (n); 
          OPERATION (m);  OPERATION (RGXOP ('}'));
          break;

        /*
        .. Non-consuming, boundary assertion patters
        */
        case '^' : case '$' :
          break;

        /*
        .. Groupings [ ], ( ) and < >  ( < > is replacement for [^] )
        */
        case '[' :
        case '<' : 
        case '(' :
          if ( last & (RGXOPD | RGXOPN) ) {
            PUSH (queue, op);
            PUSH (queue, RGXOP(';'));
            break;
          }
          OPERATOR ( op );
          break;
        case '>' : PUSH (queue, RGXOP ('~'));
        case ']' :
        case ')' :
          /*
          .. '>' = '<' + 2,
          .. ']' = '[' + 2,
          .. ')' = '(' + 1
          */
          int c = op - 2 + (op == RGXOP(')'));
          for (int i=0; i<2 && ( TOP (ostack) != c ); ++i)
            PUSH (stack, POP (ostack));
          if ( POP (ostack) != c )
            return RGXERR;                  /* depth should be <= 2 */
          last = RGXOPN;
          break;

        case ',' : /* '-' has precedence over OR (implicit) in [] */
          if ( TOP (ostack) == RGXOP('-') )
            PUSH (stack, POP(ostack));
        case ';' :
          if ( TOP (ostack) == op )
            PUSH (stack, POP(ostack));
          OPERATOR (op);
          break;

        case '|' :
          if ( TOP (ostack) == RGXOP(';') )
            PUSH (stack, POP(ostack));
          if ( TOP (ostack) == RGXOP('|') )
            PUSH (stack, POP(ostack));
          OPERATOR (op);
          break;

        case '-' :
          if (last != RGXOPD) return RGXERR;
          if (TOP(ostack) == op) PUSH (stack, POP(ostack));
          OPERATOR (op);
          break;

        default :
          return RGXERR;
      }
    }
    else {
      if ( last & (RGXOPD | RGXOPN) ) {
        PUSH (queue, op);
        PUSH (queue, RGXOP(charclass ? ',' : ';'));
        continue;
      }
      OPERAND (op);
    }
  }

  return RGXERR;

  #undef  POP
  #undef  TOP
  #undef  STACK
  #undef  PUSH
  #undef  OPERATOR
  #undef  OPERAND
  #undef  OPERATION
}

void rgx_free () {
  destroy ();
}

