%%

true                                            { return 1; }
false                                           { return 2; }
null                                            { return 3; }
\"([^"\\]|\\["\\/bfnrt]|\\u[0-9a-fA-F]{4})*\"   { return 4; }
\[                                              { return 5; }
\]                                              { return 6; }
:                                               { return 7;}
,                                               { return 8; }
\{                                              { return 9; }
\}                                              { return 10; }
-?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?  { return 11; }
[ \t\v\n\f]+                                    { /* skip */ }
.                                               { fprintf (stderr, "\nerror : %s", yytext); }

%%

int main () {
  while ( lxr_lex() ) {
    printf ("\n%s", yytext);
  }   
}
