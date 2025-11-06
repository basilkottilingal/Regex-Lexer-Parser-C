/**
c99 tokenizer
*/

O   [0-7]
D   [0-9]
NZ  [1-9]
L   [a-zA-Z_]
A   [a-zA-Z_0-9]
H   [a-fA-F0-9]
HP  (0[xX])
E   ([Ee][+-]?{D}+)
P   ([Pp][+-]?{D}+)
FS  (f|F|l|L)
IS  (((u|U)(l|L|ll|LL)?)|((l|L|ll|LL)(u|U)?))
CP  (u|U|L)
SP  (u8|u|U|L)
ES  (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
WS  [ \t\v\n\f]
STRING \"([^"\\\n]|{ES})*\"

%{

/* over ride the lxr default buff size of 16 kB */
#define  LXR_BUFF_SIZE 128
/* let's use yylex() for lexer fn as in lex/flex */
#define  YYSTYPE  int yylex (int inval)

#include "tokens.h"
static int column = 1, line = 1;
char * file;
 
%}
	 
%%

"/*"                                    {
            int c;
            while ( (c = lxr_input () ) != EOF ) {
              if ( c != '*' ) continue;
              while ( ( c = lxr_input () ) == '*' ) {}
              if ( c == '/' ) break;
            }
          }
"//".*                                  { /* skip single line comments */ }
^[ \t]*#[ \t]+[0-9]+[ \t]+{STRING}.*    { /* only for preprocessing */ }
^[ \t]*#	                              { /* only for preprocessing */
            int c, p = '#';
            while ( (c = lxr_input () ) != EOF ) {
              /* ISO C : '\\' immediately followed '\n' are ommitted */
              if ( c == '\n' ) {
                if ( p == '\\') {
                  p = '\n'; continue;
                }
                break;
              }
              p = c;
            }
          }
"__attribute__"{WS}*\(                  { /* applied only for gcc/clang */
            int c, scope = 1;
            while ( (c = lxr_input () ) != EOF ) {
              if ( c == '(' ) scope++;
              if ( c == ')' ) {
                scope--;
                if (!scope) break;
              }
            }
          }
{L}{A}*                                 {
            /* fixme : classify : keywords, id, enum, typedef*/
            return IDENTIFIER;
          }

{HP}{H}+{IS}?				                    { return I_CONSTANT; }
{NZ}{D}*{IS}?				                    { return I_CONSTANT; }
"0"{O}*{IS}?				                    { return I_CONSTANT; }
{CP}?"'"([^'\\\n]|{ES})+"'"		          { return I_CONSTANT; }

{D}+{E}{FS}?				                    { return F_CONSTANT; }
{D}*"."{D}+{E}?{FS}?			              { return F_CONSTANT; }
{D}+"."{E}?{FS}?			                  { return F_CONSTANT; }
{HP}{H}+{P}{FS}?			                  { return F_CONSTANT; }
{HP}{H}*"."{H}+{P}{FS}?			            { return F_CONSTANT; }
{HP}{H}+"."{P}{FS}?			                { return F_CONSTANT; }

({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+	      { return STRING_LITERAL; }

"..."					                          { return ELLIPSIS; }
">>="					                          { return RIGHT_ASSIGN; }
"<<="					                          { return LEFT_ASSIGN; }
"+="					                          { return ADD_ASSIGN; }
"-="					                          { return SUB_ASSIGN; }
"*="					                          { return MUL_ASSIGN; }
"/="					                          { return DIV_ASSIGN; }
"%="					                          { return MOD_ASSIGN; }
"&="					                          { return AND_ASSIGN; }
"^="					                          { return XOR_ASSIGN; }
"|="					                          { return OR_ASSIGN; }
">>"					                          { return RIGHT_OP; }
"<<"					                          { return LEFT_OP; }
"++"					                          { return INC_OP; }
"--"					                          { return DEC_OP; }
"->"					                          { return PTR_OP; }
"&&"					                          { return AND_OP; }
"||"					                          { return OR_OP; }
"<="					                          { return LE_OP; }
">="					                          { return GE_OP; }
"=="					                          { return EQ_OP; }
"!="					                          { return NE_OP; }
("{"|"<%")				                      { return '{'; }
("}"|"%>")				                      { return '}'; }
("["|"<:")	                       			{ return '['; }
("]"|":>")				                      { return ']'; }
";"|","|":"|"="|"("|")"|"."|"&"|"!"|"~"|"-"|"+"|"*"|"/"|"%"|"<"|">"|"^"|"|"|"?" {
            /* single character tokens */
            return yytext [0];
          }

{WS}+					                          { /* consume */ }
.					                              { /* catch all bad characters */ }
	 
%%


int main () {
  int tkn;

  const char str [] = 
    "char str[] = \"str\";\n"
    "/*..........................\n"
    "............................\n"
    "............................\n"
    "............................*/"
    "int x = 100;\n"
    "int func (void) {\n"
    "  return 100;\n"
    "}\n"
    ;
  lxr_read_bytes (str, strlen (str), 1);
  while ( (tkn = yylex (0)) ) {
    printf ("\n[%3d] : %s", tkn, yytext);
    fflush (stdout);
  }
  lxr_clean ();
}
