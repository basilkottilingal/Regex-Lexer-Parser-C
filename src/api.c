

/*
.. The list of api defined in lxr
.. (a) lxr_source() set the source file before starting the lexing.
..     If lxr_infile is not set by lxr_source(), assumes stdin as the
..     source of .lex grammar.
.. (b) lxr_input () return the next byte from the input source and
..     move the reading pointer.
..     Return value in [0x00, 0xFF] or EOF
.. (c) lxr_unput () return previous byte in [0x00, 0xFF].
..     Limit : You can unput a maximum of yyleng characters (and the
..     recently emulated lxr_input () characters).
..     Returns EOF, in case the pointer reaches yytext.
.. (d) lxr_clean() : clean the stack of buffers.
..     Call at the end of the program
.. (e) lxr_stack_push () : You can change the source file in the
..     middle of lexing, by calling this function.
.. (f) lxr_stack_pop () go back to the previous source stream.
..     May pass "isdestroy" = 1, if you want to destroy all the
..     buffers holding current source stream.
*/

void lxr_source     ( const char * source );
int  lxr_input      ( );
int  lxr_unput      ( );
void lxr_clean      ( );
void lxr_stack_push ( const char * source );
void lxr_stack_pop  ( int isdestroy );




