# Overview
 
  Interested in developing lexers ( read source code token by token ) ?
  Here is a minimal effort. (Not yet complete).

  Breaking down.
  1. regular expression (regex) are used to represent tokens.
  2. Non-deterministic tokens are build from regex
  3. Deterministic tokens are build from 

## Tokenizer or Lexer reader

  Creates a lexer generator header from a .lex file. Input file supports
  same format as of flex's input file.

  Convert a lexer grammar file to lexer source file as follows.

```bash
git clone git@github.com:basilkottilingal/lexer.git
cd lexer
make lxr
./lxr -o c99.c c99.lex
```

  (fixme : export to bashrc. Might need LXRPATH also!!)

### Regex

  1. Support most of POSIX ERE symbols in the lexer grammar.
  2. Following regular expression are implemented

| Symbol   | Name         | Meaning / Description                                                   |
| -------- | ------------ | ----------------------------------------------------------------------- |
| `.`      | Dot  | Matches **any single character** except newline (`\n`)                          |
| `^`      | Beginning anchor | Matches the **start of a line**                                     |
| `$`      | End anchor   | Matches the **end of a line**                                           |
| `[...]`  | Character class  | Matches **any one character** in the set                            |
| `[^...]` | Negated character class  | Matches **any one character not** in the set                |
| `\|`     | Alternation  | Matches **either of the pattern** before o after `\|` in the current group |
| `()`     | Grouping     | Groups expressions as a single unit, affects alternation and repetition |
| `*`      | Zero or more | Matches **zero or more** repetitions of the preceding expression        |
| `+`      | One or more  | Matches **one or more** repetitions of the preceding expression         |
| `?`      | Zero or one  | Matches **zero or one** occurrence of the preceding expression          |
| `{n}`    | Exact repetition | Matches **exactly n** occurrences of the preceding expression       |
| `{n,}`   | Lower-bounded repetition | Matches **n or more** occurrences                           | 
| `{,m}`   | Upper-bounded repetition | Matches **atmost m** occurrences                            | 
| `{n,m}`  | Bounded repetition   | Matches **between n and m** occurrences (inclusive)             |
| `\`      | Escape | Removes special meaning from the next character (e.g., `\.` matches a literal `.`) |

  3. NOTE : DOESN'T support any kind of pattern lookahead assertions.
```lex
foo\bar    { /* foo should be followed by bar */ }
foo(?=bar) { /* foo should be followed by bar */ } 
foo(?!bar) { /* foo shouldn't be followed by bar */ } 
```
  So it's upto user to handle these look ahead patterns by appropriate alternative methods
  like using appropriate context sensitive parser, or implementing a lookahead
  using lxr\_input()/lxr\_unput()

  4. NOTE : DOESN'T support EOF as in flex
```lex
<<EOF>>    { /* flex allows EOF action, but not allowed in this code */ }
```

## Regex pattern matching using NFA.

  The same script can be used to look if a text satisfies regex pattern
```c
  #include "regex.h"
  #include <stdio.h>

  int main () {
    char rgx[] = "(a|b)*c";
    char str[] = "aac";
    int len = rgx_nfa_match (nfa, "aac"); // returns > 0, if successful
    if (len) {
      char holdchar = str [len-1];
      str [len-1] = '\0';
      printf ("substring %s matched pattern %s", str, rgx);
      str [len-1] = holdchar;
    }
  }
```
