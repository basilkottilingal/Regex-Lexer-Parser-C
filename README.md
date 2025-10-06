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

  Creates a lexer generator header from a .lex file. Input file supports
  same format as of flex's input file.

### Regex

  Support most of POSIX ERE symbols
| Symbol             | Name                     | Meaning / Description                                                              |                                                         |   |
| ------------------ | ------------------------ | ---------------------------------------------------------------------------------- | ------------------------------------------------------- | - |
| `.`                | Dot                      | Matches **any single character** except newline (`\n`)                             |                                                         |   |
| `^`                | Beginning anchor         | Matches the **start of a line**                                                    |                                                         |   |
| `$`                | End anchor               | Matches the **end of a line**                                                      |                                                         |   |
| `[...]`            | Character class          | Matches **any one character** in the set                                           |                                                         |   |
| `[^...]`           | Negated character class  | Matches **any one character not** in the set                                       |                                                         |   |
| `                  | `                        | Alternation                                                                        | Matches either the **expression before or after** the ` | ` |
| `()`               | Grouping                 | Groups expressions as a single unit, affects alternation and repetition            |                                                         |   |
| `*`                | Zero or more             | Matches **zero or more** repetitions of the preceding expression                   |                                                         |   |
| `+`                | One or more              | Matches **one or more** repetitions of the preceding expression                    |                                                         |   |
| `?`                | Zero or one              | Matches **zero or one** occurrence of the preceding expression                     |                                                         |   |
| `{n}`              | Exact repetition         | Matches **exactly n** occurrences of the preceding expression                      |                                                         |   |
| `{n,}`             | Lower-bounded repetition | Matches **n or more** occurrences                                                  |                                                         |   |
| `{n,m}`            | Bounded repetition       | Matches **between n and m** occurrences (inclusive)                                |                                                         |   |
| `\`                | Escape                   | Removes special meaning from the next character (e.g., `\.` matches a literal `.`) |                                                         |   |


