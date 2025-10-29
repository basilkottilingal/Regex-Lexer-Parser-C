# Overview
 
  Interested in developing lexers ( read source code token by token ) ?
  Here is a minimal effort.

  The script can be used to create lexer source file like unix lex/
  GNU [flex](https://github.com/westes/flex) (note : flex is no more
  an official GNU package). For portability, it accepts flex's grammar
  file with few exceptions in the patterns.
  The main API function are

```c
void lxr_source     ( const char * source );
int  lxr_input      ( );
int  lxr_unput      ( );
void lxr_clean      ( );
```

# References, Read More
  1. regular expression (regex) are converted to NFA using [Thompson's NFA 
construction](https://dl.acm.org/doi/abs/10.1145/363347.363387). Minimal
implementation in C is [here](https://swtch.com/~rsc/regexp/regexp1.html)
  2. NFA to DFA conversion and minimisation of DFA transition Table
using [Hopcroft's algorithm](https://www.sciencedirect.com/science/article/abs/pii/B9780124177505500221)
  3. Equivalence classes
  4. (b) Table-Based Lexers (DFA Execution Models)
```
Aho, A. V., Sethi, R., and Ullman, J. D.
Compilers: Principles, Techniques, and Tools (1st ed., 1986) — “The Dragon Book”.
```
  5. Table compression Techniques
```
Aho, Alfred V., and Ullman, Jeffrey D.
“Compressed Representation of Finite Automata.”
Proceedings of the 3rd Annual ACM Symposium on Theory of Computing (STOC), 1971, pp. 116–123.
```

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
