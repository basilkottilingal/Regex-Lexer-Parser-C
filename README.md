# Overview
 
  Interested in developing lexers ( read source code token by token ) ?
  Here is a minimal effort. (Not yet complete).

  Breaking down.
  (a) regular expression (regex) are used to represent tokens.
  (b) Non-deterministic tokens are build from regex
  (d) Deterministic tokens are build from 

## Regex pattern matching using NFA.

  To look if a text satisfies regex pattern
```c
char rgx[] = "(a|b)*c";
int len = rgx_nfa_match (nfa, "aac"); // returns > 0, if succesul
```

## Tokenizer or Lexer reader

