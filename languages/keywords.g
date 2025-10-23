/*
.. Run this gperf source code using
.. $ gperf -L C -t keywords.g > keywords.c
*/

%{
  #include "tokens.h"
%}

struct Keyword { const char *name; int token; };
%struct-type
%language=C
%readonly-tables
%define slot-name name

%%
auto,      AUTO
break,     BREAK
case,      CASE
char,      CHAR
const,     CONST
continue,  CONTINUE
default,   DEFAULT
do,        DO
double,    DOUBLE
else,      ELSE
enum,      ENUM
extern,    EXTERN
float,     FLOAT
for,       FOR
goto,      GOTO
if,        IF
inline,    INLINE
int,       INT
long,      LONG
register,  REGISTER
restrict,  RESTRICT
return,    RETURN
short,     SHORT
signed,    SIGNED
sizeof,    SIZEOF
static,    STATIC
struct,    STRUCT
switch,    SWITCH
typedef,   TYPEDEF
union,     UNION
unsigned,  UNSIGNED
void,      VOID
volatile,  VOLATILE
while,     WHILE
_Alignas,  ALIGNAS
_Alignof,  ALIGNOF
_Atomic,   ATOMIC
_Bool,     BOOL
_Complex,  COMPLEX
_Generic,  GENERIC
_Imaginary,  IMAGINARY
_Noreturn,   NORETURN
_Static_assert,  STATIC_ASSERT
_Thread_local,   THREAD_LOCAL
__func__,        FUNC_NAME
%%
