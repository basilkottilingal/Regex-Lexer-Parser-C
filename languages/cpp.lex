/**
c preprocessor tokenizer
*/

ES  (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
WS  ([ \t]|\\\n)
STRING \"([^"\\\n]|{ES})*\"

%{

#include "tokens.h"
static int column = 1, line = 1;
static char * file;
void pp_location ();
void pp_std_header ();
void pp_local_header ();
 
%}
	 
%%

"/*"                                               {
            int c;
            while ( (c = lxr_input () ) != EOF ) {
              if ( c != '*' ) continue;
              while ( (c = lxr_input ()) == '*' ) {}
              if ( c == '/' ) break;
            }
          }
"//".*                                             { }
^[ \t]*#[ \t]{WS}*[0-9]+[ \t]{WS}*{STRING}.*               {
            pp_location ();
          }
^[ \t]*#{WS}*"include"{WS}*<[a-z_A-Z0-9/.\\+~-]+>.*    {
            /* relative path headers */
            printf ("\n%s:", yytext);
            pp_std_header ();
          }
^[ \t]*#{WS}*"include"{WS}*{STRING}.*    {
            /* mostly a std header */
            printf ("\n%s:", yytext);
            pp_local_header ();
          }
^[ \t]*#	                                         {
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
            printf ("\n%s:", yytext);
          }
.					                              { /* catch all bad characters */ }
	 
%%


void pp_location () {
  printf ("\n location");
}

void pp_std_header () {
  printf ("\n include <.h>");
}

void pp_local_header () {
  printf ("\n include \".h\"");
}

int main () {
  int tkn;
  while ( (tkn = lxr_lex()) ) {
    printf ("\n[%3d] : %s", tkn, yytext);
  }
}
