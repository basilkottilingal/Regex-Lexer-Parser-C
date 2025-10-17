/**
c preprocessor tokenizer
*/

S       [ \t]
WS      ([ \t]|\\\n)
ID      ([_a-zA-Z][_a-zA-Z0-9]*)
ES      (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
STRING  \"([^"\\\n]|{ES})*\"

%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int  column = 1, line = 1;
static char * file = NULL;
static char * source = NULL;
static void location     ( );
static void std_header   ( );
static void local_header ( );

enum STATUS {
  IF,
  ELSE,
  DEFINE,
  NONE 
};

static int pstatus = NONE;
 
%}
	 
%%

"/*"                                                   {
            int c;
            while ( (c = lxr_input () ) != EOF ) {
              if ( c != '*' ) continue;
              while ( (c = lxr_input ()) == '*' ) {}
              if ( c == '/' ) break;
            }
          }
"//".*                                                 { }
^[ \t]*#[ \t]{WS}*[0-9]+[ \t]{WS}*{STRING}.*           {
            printf ("\nfile-line %s:", yytext);
            location ();
          } 
^[ \t]*#{WS}*"include"{WS}*<[a-z_A-Z0-9/.\\+~-]+>.*    {
            /* mostly a std header */
            printf ("\nstd.h %s:", yytext);
            std_header ();
          }
^[ \t]*#{WS}*"include"{WS}*{STRING}.*                  {
            /* relative path headers */
            printf ("\nheader %s:", yytext);
            local_header ();
          }
^[ \t]*#{WS}*"if"                                      {
            printf ("\nif %s:", yytext);
          }
^[ \t]*#{WS}*"elif"                                      {
            printf ("\nelif %s:", yytext);
          }
^[ \t]*#{WS}*"else".*                                  {
            printf ("\nelse %s:", yytext);
          }
^[ \t]*#{WS}*"endif".*                                 {
            printf ("\nendi %s:", yytext);
          }
^[ \t]*#{WS}*"define"[ \t]{WS}*{ID}{WS}+               {
            printf ("\ndefine 1 %s:", yytext);
          }
^[ \t]*#{WS}*"define"[ \t]{WS}*{ID}\(                  {
            printf ("\ndefine 2 %s:", yytext);
          }
^[ \t]*#{WS}*"undef"[ \t]{WS}*{ID}.*                   {
            printf ("\nundef %s:", yytext);
          }
^[ \t]*#	                                             {
            int c, p = '#';
            while ( (c = lxr_input () ) != EOF ) {
              /* ISO C : '\\' immediately followed by '\n' are ommitted */
              if ( c == '\n' ) {
                if ( p == '\\') {
                  p = '\n'; continue;
                }
                break;
              }
              p = c;
            }
            lxr_token ();
            printf ("\nother %s:", yytext);
          }
(.|[\n])			                              { /* catch all bad characters */ }
	 
%%

static
void location () {
}

static
void std_header () {
}

static
void local_header () {
}

int main ( int argc, char * argv[] ) {

  
  
  int tkn;
  while ( (tkn = lxr_lex()) ) {
    printf ("\n[%3d] : %s", tkn, yytext);
  }
}
