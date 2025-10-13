/*
.. Read a lex file. Follows the flex's ".lex" format. You can refer to
.. the flex documentation on the input file format here
.. https://westes.github.io/flex/manual/Format.html#Format
.. Input file need to follow the format
..
.. definitions
.. %%
.. rules
.. %%
.. user code
..
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"
#include "allocator.h"
#include "lexer.h"
#include "regex.h"
#include "class.h"

#define LXR_DEBUG

static FILE * out;

/*
.. Flex macro that holds a regex pattern. Macro naming should follow
.. the format [_a-zA-Z][_a-zA-Z0-9]*. You can recurse macro, i.e a
.. macro can use a previous macro, in it's definition. Macros are
.. defined in the definition section. Examples of macros are below
.. 
.. ALPH   [_a-zA-Z]
.. DIGITS [0-9]
.. ID     {ALPHA}({ALPH}|{DIGITS})*
*/

typedef struct Macro {
  char * key, * rgx;
  struct Macro * next;
} Macro;

/*
.. A code snippet is defined in the definitions section in either of
.. the two forms as given below
.. 
.. %{
..   #include <stdio.h>
..   #include "func.h"
.. %}
*/

typedef struct Snippet {
  char * code;
  int line;
  struct Snippet * next;
} Snippet;

static Snippet * snippet_head = NULL;
static Snippet ** snippets = & snippet_head;

/*
.. patterns are defined exclusively in the rules section, that falls
.. in between two %% separators. The pattern should be followed by the
.. corresponding action. Pattern is defined in the POSIX extended
.. regular expressions (ERE) with few exceptions
.. (a) strings are allowed in quotes
.. (b) macros inside {}
.. (c) . (dot or "all-character" class) excludes '\n'
.. (d) anchors '^' and '$' are allowed only at the beginning/end of
..     the tokens respectively 
..
.. %%
.. ID       { return IDENTIFIER; }
.. %%
*/
typedef struct Action {
  char * rgx, * action;                    /* Pattern - Action pair */
  int line;
} Action;

static Stack * actions = NULL;

/* ...................................................................
.. ...................................................................
.. ..................  hash table of macros  .........................
.. ...................................................................
.. .................................................................*/

#define prime 67u
static Macro * table [prime];

static inline 
uint32_t hash ( const char * key, uint32_t len ) {
  	
  /* 
  .. Seed is simply set as 0
  */
  uint32_t h = 0;
  uint32_t k;

  /*
  .. Dividing into blocks of 4 characters.
  */
  for (size_t i = len >> 2; i; --i) {
    memcpy(&k, key, sizeof(uint32_t));
    key += sizeof(uint32_t);

    /*
    .. Scrambling each block 
    */
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;

    h ^= k;
    h = ((h << 13) | (h >> 19)) * 5 + 0xe6546b64;
  }

  /*
  .. tail characters.
  .. This is for little-endian.  You can find a different version 
  .. which is endian insensitive.
  */  
  k = 0; 
  switch (len & 3) {
    case 3: 
      k ^= key[2] << 16;
    case 2: 
      k ^= key[1] << 8;
    case 1: 
      k ^= key[0];
      k *= 0xcc9e2d51;
      k = (k << 15) | (k >> 17);
      k *= 0x1b873593;
      h ^= k;
  }

  /* 
  .. Finalize
  */
  h ^= len;
  h ^= (h >> 16);
  h *= 0x85ebca6b;
  h ^= (h >> 13);
  h *= 0xc2b2ae35;
  h ^= (h >> 16);

  /* return murmur hash */
  return h;
}

/*
.. lookup for a macro in the macro hashtable "table"
*/
static Macro ** lookup (const char * key) {
  uint32_t h = hash (key, strlen (key));
  Macro ** p = & table [h % prime], * m;
  while ( (m = *p) != NULL ) {
    if ( !strcmp (m->key, key) )
      return p;
    p = & m->next;
  }
  return p;
}

/* ...................................................................
.. ...................................................................
.. .....  reading definition section ( macros & header code )  .......
.. ...................................................................
.. .................................................................*/


static int line = 0;
/*
.. Read a new macro and insert it into the hashtable and corresponding
.. regex pattern
*/
static int macro_new (char * macro, char * pattern) {

  #define COPY(start) do {                                           \
      size_t l = ptr - start;                                        \
      if (l + len >= RGXSIZE) {                                      \
        error ("rgx buffer out of memory");                          \
        return RGXERR;                                               \
      }                                                              \
      memcpy (& rgx [len], start, l);                                \
      len += l;                                                      \
    } while (0)

  Macro ** p, * m;
  char rgx [RGXSIZE], * ptr = pattern, c;
  size_t len = 0;

  while ( (c = *ptr++) ) {

    char * start = ptr-1;
    if (c == '\\') {
      if ( (c = *ptr++) == '\0' ) {
        error ("incomplete escaping");
        return RGXERR;
      }
      COPY (start);
      continue;
    }
    if (c != '{') {
      COPY (start);
      continue;
    }
    if ( (c = *ptr++) == ',' || ( c <= '9' && c >= '0' ) ) {
      COPY (start);
      continue;
    }

    char * child = ptr - 1;
    if ( !(c == '_' || (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z')) )
      return RGXERR;

    do {
      /*
      .. In case the macro uses another macro, substitute the child
      .. macro by the corresponding regex
      */
      if ( c == '}' ) {
        ptr[-1] = '\0';
        p = lookup (child);
        ptr[-1] = '}';
        if ( (m = *p) == NULL ) {
          error ("macro {%s} not found", child);
          return RGXERR;
        }
        size_t l = strlen (m->rgx);
        if (l + len >= RGXSIZE) {
          error ("rgx buffer out of memory"); 
          return RGXOOM;
        }
        memcpy ( & rgx [len], m->rgx, l );
        len += l;

        break;
      }

      if ( c == '_' || (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') )
        continue;

      error ("invalid child macro name");
      return RGXERR;

    } while ( (c = *ptr++) );

    if (!c) {
      error ("incomplete }");
      return RGXERR;
    }
  }

  /*
  .. Check the validity of the macro's regex pattern before inserting
  .. to the table
  */
  rgx [len] = '\0';
  int rpn [RGXSIZE];
  len = rgx_rpn (rgx, rpn);       /* return rgx length if succesful */
  if (len <= 0) {
    error ("cannot create rpn for regex %s in line %d", rgx, line);
    return RGXERR;
  }
  rgx [len] = '\0';        /* Remove trailing white spaces (if any) */

  /*
  .. Insert macro to hashtable
  */
  p = lookup (macro);
  if ( (m = *p) != NULL ) {
    error ( "macro %s redefined in lexer definition in line %d",
      macro, line);
    return RGXERR;
  }
  *p = m = allocate (sizeof (Macro));
  m->key = allocate_str (macro);
  m->rgx = allocate_str (rgx);

  if ( ! ( (rgx [0] == '[' && rgx [len-1] == ']') ||
       (rgx [0] == '(' && rgx [len-1] == ')') ) ) {
    error ("warning : bracket, ( ), suggested for macro %s", macro);
  }
  #if 0
  printf ("\nmacro %s rgx %s", macro, rgx); fflush (stdout);
  #endif
  
  return 0;
}

/*
.. Read definition section in the lexer file. The beginning section
.. until first ^%%, is called the definition where you define regex
.. macros and pre-lexer code snippets
*/
int lex_read_definitions ( FILE * in ) {

  char buff [PAGE_SIZE], *ptr, c;
  int code = 0, comment = 0;

  while (fgets (buff, sizeof buff, in)) {       /*read line by line */
    line++;

    size_t len = strlen (buff);

    if ( buff [ len-1 ] != '\n' && !feof (in) ) {
      error ("very long line");
      return RGXOOM;
    }

    if (code) {                   /* .. Source code section ^%{ ^%} */
      if ( buff [0] == '%' && buff [1] == '}' ) {
        code = 0; continue;
      }
      Snippet * s = allocate ( sizeof (Snippet) );
      s->code = allocate_str (buff);
      s->line = line;
      *snippets = s;
      snippets = & s->next;
      continue;
    }

    ptr = buff; c = *ptr++;
    if ( c == '/' && !comment ) {  /* See if beginning of a comment */
      if ( (c = *ptr++) != '*' ) {
        error ("unknown syntax");
        return RGXERR;
      }
      c = *ptr++; comment = 1;
    }
    if (comment || c == ' ' || c == '\t' || c == '\n') {
      do {         /* consume comments, white spaces and empty line */
        if (c == '\n') break;
        if (comment) {
          if (c == '*' && *ptr++ == '/') comment = 0;
          continue;
        }
        if ( c == '/' && *ptr == '*' ) {
          ++ptr; comment = 1; continue;
        }
        if (c == ' ' || c == '\t') continue;

        error ("content should be placed at BOL");
        return RGXERR;
      } while ( (c = *ptr++) );
      continue;
    }

    if ( c == '%' ) {
      c = *ptr++;
      if (c == '{' ) {      /* %{ is the beginning of code snippets */
        code = 1;
        continue;
      }
      if (c == '%')          /* %% is the end of definition section */
        return 0;
      error ("warning : unknown/unimplemented in line %d", line);
      continue;  /* Other % option, %top {}, etc .. not implemented */
    }

    /*
    .. A macro definition, maps a macro identifier to a rgx fragment.
    */
    if ( c == '_' || (c <='z' && c >= 'a') ||
       (c <='Z' && c >= 'A') ) { 
      while ( (c = *ptr++) != '\n' ) {
        if ( c == '_' || (c <='z' && c >= 'a') ||
          (c <='Z' && c >= 'A') || (c <='9' && c >= '0') )
          continue;
        if ( ! (c == ' ' || c == '\t' ) ) {
          error ("missing separator");
          return RGXERR;
        }
        ptr [-1] = buff [len - 1] = '\0';
        while ( (c = *ptr++) == ' ' || c == '\t' ){}
        --ptr;
        int status = macro_new (buff, ptr);
        if (status < 0) {
          error ("cannot create macro error %s", buff);
          return RGXERR;
        }
        break;
      }
      if ( c == '\n' ) {
        error ("ill defined macro");
        return RGXERR;
      }
      continue;
    }

    /*
    .. Anything other than (i) %**, (ii) macro definition, (iii) empty
    .. white spaces and/or comments are taken as unknown syntax
    */
    error ("unknown syntax");
    return RGXERR;

  }

  error ("Did not encounter %%");
  return RGXERR;
}

/* ...................................................................
.. ...................................................................
.. ........  reading rule section ( rules & action )  ................
.. ...................................................................
.. .................................................................*/

/*
.. Naive way to look for the regex pattern at the beginning of the
.. line. The regex will be stored in the buffer "rgx".
*/
static
int _pattern (FILE * in, char * rgx, size_t lim) {
  int c, len = 0, escape = 0, charclass = 0, quote = 0, end = 0;
  #define MACROLIM 128
  char macro [MACROLIM];
  while ( (c = fgetc(in)) != EOF ) {
    rgx[len++] = (char) c;
    if (len == lim - 2) {
      error ( "rgx buff limit" );
      return RGXOOM;
    }
    if (escape) {
      escape = 0;
      continue;
    }
    if (c == '\\') {
      escape = 1;
      continue;
    }
    if (quote) {
      switch (c) {
        case '"' : len--; quote = 0; break;
        case '|' : case '*' : case '?' : case '+' :
        case '[' : case ']' : case '{' : case '}' :
        case '(' : case ')' : case '^' : case '$' :
        case '-' : case '.' : case '\\' :
          rgx [len-1] = '\\'; rgx [len++] = c; break;
      }
      continue;
    }
    if (c == '\n') {
      ++line;
      #define EMPTY 10
      #define NOACTION 11
      if ( len == 1 )
        return EMPTY;                                 /* Empty line */
      rgx[--len] = '\0';
      return NOACTION;                    /*regex with no action !! */
    }
    if (charclass) {
      if (c == ']')
        charclass = 0;
      continue;
    }

    switch (c) {    /* outside quotes "" and outside character class*/
      case '/' :
        error ("warning : (in case) look ahead operator '/' is "
          "not implemented. line %d", line);
        break;
      case '[' : charclass = 1; break;
      case ' ' :
      case '\t' :          /* White space are taken as end of regex */
        rgx[--len] = '\0';
        if ( len == 0 ) {
          int comment = 0, p = c;
          while ( (c = fgetc(in)) != EOF ) {
            if ( c == '\n' ) { 
              ++line;
              if (!comment) break;
            }
            if ( comment ) {
              if (c == '/' && p == '*' ) comment = 0;
            }
            else if ( c == '/' ) {
              if ( (c = fgetc (in)) != '*' )
                return RGXERR;
              comment = 1;
            }
            else if ( !(c == ' '|| c == '\t') )  {
              return RGXERR;
            }
            p = c;
          }
          if ( c == EOF )
            return RGXERR;
          return EMPTY;       /* Empty line, with just white spaces */
        }
        #if 0
        printf ("\npattern  %s", rgx); fflush (stdout);
        #endif
        return 0;
      case '%' :
        if (len == 2 && end)
          return EOF;
        if (len == 1) end = 1;
        break;
      case '{' :
        --len;
        int k = 0;
        c = fgetc (in);
        if (c == ',' || (c >= '0' && c <= '9')) {
          rgx [len++] = c;
          continue;
        }
        if ( !(c == '_' || (c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z')) )
          return RGXERR;
        do {
          if (c == '}') break;
          if (k == MACROLIM-2) return RGXOOM;
          if ( c == '_' || (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') )
            {
              macro [k++] = c;
              continue;
            }
          return RGXERR;
        } while ( (c = fgetc (in)) != EOF );
        if ( c!= '}') return RGXERR;
        macro [k] = '\0';
        Macro ** p = lookup (macro), *m;
        if ( (m = *p) == NULL ) {
          error ("macro {%s} not found", macro);
          return RGXERR;
        }
        int l = strlen (m->rgx);
        if (l + len >= lim) {
          error ("rgx buffer out of memory"); 
          return RGXOOM;
        }
        memcpy ( & rgx [len], m->rgx, l );
        len += l;
        break;
      case '"' :
        --len; quote = 1;
        break;
    }
  }

  if (!len) {
    error ("Did not encounter %%");
    return RGXERR;
  }

  error ("Incomplete rgx or missing action");
  return RGXERR;
}

/*
.. Naive way to read action. The action should be put inside a '{' '}'
.. block and the starting '{' should be placed in the same line of
.. the regex pattern
*/
static
int _action (FILE * in, char * buff, size_t lim) {
  int c, j = 0, depth = 0, quote = 0, escape = 0;

  /*
  .. Consume white spaces before encountering '{' i.e start of action.
  .. Warning : Action need to be in the same line as regex.
  */
  while ( (c = fgetc(in)) != EOF ) {
    if ( c == '{' ) {
      depth++;
      buff [j++] = '{';
      break;
    }
    if ( !(c == ' ' || c == '\t' ) ) {
      error ("lxr grammar : Unexpected character"
        "between regex & action");
      return RGXERR;
    }
  }

  /*
  .. Doesn't validate C grammar that falls in action block { ... }.
  .. The whole action block will be stored inside 'buff' unless
  .. you run out of limit.
  */
  while ( (c = fgetc(in)) != EOF ) {
    buff[j++] = (char) c;
    if (j == lim) {
      error ( "action buff limit" );
      return RGXERR;
    }

    if (c == '\n')
      line++;

    if (quote) {
      if (escape)
        escape = 0;
      else if (c == '\\')
        escape = 1;
      else if (c == '"') {
        quote  = 0;
        continue;
      }
    }
    else {
      if (c == '"') {
        quote = 1;
        continue;
      }
      if (c == '{')
        depth++;
      else if (c == '}') {
        depth--;
        if (!depth)
          break;
      }
    }
  }
  buff[j] = '\0';

  if (depth) {
    error ("Incomplete action. Missing '}'");
    return RGXERR;
  }

  /*
  .. Consume white spaces after the action '{' .. '}'
  */
  while ( (c = fgetc(in)) != EOF ) {
    if ( c == '\n') {
      line++;
      break;
    }
    if ( ! (c == ' ' || c == '\t' ) ) {
      error ("unexpected character after action");
      return RGXERR;
    }
  }
  return 0;
}

static inline 
void pattern_action ( const char * r, const char * a, int line) {
  Action * A = allocate (sizeof (Action));
  *A = (Action) { allocate_str (r), allocate_str (a), line };
  stack_push (actions, A);
}

int lex_read_rules ( FILE * in ) {

  actions = stack_new (0);

  int status, cline;
  char rgx [RGXSIZE], action [PAGE_SIZE];

  for (;;) {
    cline = line;
    /* Read rgx pattern */
    status = _pattern (in, rgx, sizeof (rgx));
    if ( status == EOF ) break;
    if ( status == EMPTY ) continue;
    if ( status == RGXERR ) {
      error ("failed reading rule-action near line %d", line);
      return RGXERR;
    }
    if ( status == NOACTION ) {
      pattern_action (rgx, "", cline);
      continue;
    }
    #undef EMPTY
    #undef NOACTION

    /* Read the action for the above regex pattern */
    status = _action (in, action, sizeof (action));
    if ( status == RGXERR ) {
      error ("failed reading rule-action near line %d", line);
      return RGXERR;
    }

    pattern_action (rgx, action, cline);
  }
  return 0;  
}

/*
.. Create DFA from regex patterns, create the compressed tables for
.. lexical analysis and print the tables
*/
int lex_print_tables () {

  int nrgx = (int) ( actions->len / sizeof (void *) );
  char ** rgx = allocate ( nrgx * sizeof (char *) );
  Action ** A = (Action **) actions->stack;
  for (int i=0; i<nrgx; ++i)
    rgx [i] = A[i]->rgx;
  
  DState * dfa = NULL;
  if (rgx_lexer_dfa (rgx, nrgx, &dfa) < 0) {       /* Minimised DFA */
    error ("failed to create a minimal dfa");
    return RGXERR;
  }

  int ** tables, * len;
  if (dfa_tables (&tables, &len) < 0) {  /* comprssd tbles from DFA */
    error ("Table size Out of memory limit");
    return RGXOOM;
  }

  char * names [] = {
    "check", "next", "base", "accept", "def", "meta", "class"
  };

  char * type [] = {
    "short", "short", "short", "short", "short",
    "unsigned char", "unsigned char"
  };

  int nclass = len [5], * class = tables [6];
   
  fprintf ( out, 
    "\n/*"
    "\n.. Equivalence classes for alphabets in [0x00, 0xFF] are"
    "\n.. stored in the table lxr_class []. The number of equivalence"
    "\n.. classes also counts the special equivalence classes like"
    "\n.. that of EOB/EOL/EOF/BOL. However BOL transition is never"
    "\n.. used, as the state 2 is reserved for the starting DFA in"
    "\n.. case of BOL status."
    "\n*/"
    "\n#define lxr_nclass       %3d          /* num of eq classes */"
    "\n#define lxr_nel_class    %3d          /* new line  '\\n'    */"
    "\n#define lxr_eob_class    %3d          /* end of buffer     */"
    "\n#define lxr_eol_class    %3d          /* end of line       */"
    "\n#define lxr_eof_class    %3d          /* end of file       */",
    nclass, class ['\n'], EOB_CLASS, EOL_CLASS, EOF_CLASS );

  fprintf ( out, 
    "\n\n/*"
    "\n.. Accept state used internally. States [1, %d] are reserved"
    "\n.. for tokens listed in the lex source file"
    "\n*/"
    "\n#define lxr_eof_accept   %3d   /* end of file       */"
    "\n#define lxr_eob_accept   %3d   /* end of buffer     */",
    nrgx, nrgx + 1, nrgx + 2 );

  /*
  .. write all tables, before main lexer function
  */
  for (int i=0; i<7; ++i) {
    int * arr = tables [i], l = len [i];
    if (!arr) continue;
    fprintf ( out, "\n\nstatic %s lxr_%s [%d] = {\n",
      type [i], names[i], l );
    for (int j=0; j<l; ++j) {
      fprintf ( out, " %4d%s", arr[j], j == l-1 ? "" : ",");
      if (j%10 == 0)  fprintf (out, "\n");
      if (j%100 == 0) fprintf (out, "\n");
    }
    fprintf ( out, "\n};" );
  }

  return 0;
}

int lex_print_lxr_fnc () {
  char buf[BUFSIZ];
  size_t n;
  FILE *in = fopen("../src/tokenize.c", "r");

  if (!in)
    return RGXERR;

  while (fgets (buf, sizeof buf, in)) {
    size_t l = strlen (buf);
    if (buf [l-1] != '\n') {
      error ("insuff buff"); return RGXOOM;
    }
    int j = 0; char c;
    while ( (c = buf [j++]) != '\0' ) {
      if ( c == '/' && buf [j] == '*' && buf [j+1] == '%' ) {
        j = -1; break;
      }
    }
    if (j == -1) break;
    if (fputs (buf,out) == EOF)
      return RGXERR;
  }
  size_t nrgx = ( actions->len / sizeof (void *) );
  Action ** A = (Action **) actions->stack;
  for (int i=0; i < nrgx; ++i) {
    fprintf (out, "\n      case %d :", i+1);
    #ifdef LXR_DEBUG
    (void) A;
    fprintf (out,
      "\n        printf (\"\\nl%%3d c%%3d: token [%3d] %%s\","
      "\n          lxr_line_no, lxr_col_no, yytext);"
      "\n        break;", i+1);
    #else
    fprintf (out,
      "\n        %s\n        break;", A[i]->action);
    #endif
  }

  fprintf (out, "\n");
  while ((n = fread(buf, 1, sizeof buf, in)) > 0) {
    if (fwrite(buf, 1, n, out) != n)
      return RGXERR;
  }

  fclose(in);
  return 0;
}

void lex_print_snippets () {
  Snippet * s = snippet_head;
  while (s) {
    fprintf (out, "%s", s->code);
    s = s->next;
  }
}
  
int lex_print_source () {
  char buf[BUFSIZ];
  size_t n;
  FILE * in = fopen("../src/source.c", "r");
  if (!in)
    return RGXERR;

  while ((n = fread(buf, 1, sizeof buf, in)) > 0) {
    if (fwrite(buf, 1, n, out) != n)
      return RGXERR;
  }

  fclose(in);
  return 0;
}

int lex_print_last (FILE * in) {
  char buff [BUFSIZ];
  size_t n;
  while ((n = fread(buff, 1, sizeof buff, in)) > 0) {
    if (fwrite(buff, 1, n, out) != n)
      return RGXERR;
  }
  return 0;
}

int read_lex_input (FILE * fp, FILE * _out) {
  if (!fp) {
    error ("missing in/out");
    return RGXERR;
  }
  out = _out ? _out : stdout;

  line = 0;

  if (lex_read_definitions (fp) < 0) {
    error ("failed reading defintion section. line %d", line);
    return RGXERR;
  }

  if (lex_read_rules (fp) < 0) {
    error ("failed reading rules section. line %d", line);
    return RGXERR;
  }

  errors (); /* Flush any warnings */

  lex_print_snippets ();

  if (lex_print_tables () < 0) {
    error ("failed creating tables");
    return RGXERR;
  }

  if (lex_print_source () < 0) {
    error ("lxr : failed writing lexer head");
    return RGXERR;
  }

  if (lex_print_lxr_fnc () < 0) {
    error ("failed writing lexer function");
    return RGXERR;
  }

  lex_print_last (fp);

  #ifdef LXR_DEBUG
  fprintf (out,
    "\nint main () {\n  lxr_lex ();\n  return 0;\n}" );
  #endif

  return 0;
}
