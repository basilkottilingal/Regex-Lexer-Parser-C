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
  int flag;
  struct Macro * next;
} Macro;

/*
.. A code snippet is defined in the definitions section in either of
.. the two forms as given below
..
.. %top{
..   #include <stdio.h>
..   #include "func.h"
.. }
.. 
.. %{
..   #include <stdio.h>
..   #include "func.h"
.. %}
..
.. The difference is that %top{} snippets will be forcefully placed at
.. the top and the %{%} snippet at the bottom. So if multiple %top{}
.. snippets appear, the last one in the lex file will be forced on the
.. top of the generated lexer file.
*/

typedef struct Snippet {
  char * code;
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

typedef struct Pattern {
  char * pattern, * action;
  int id, flag;
} Pattern;

static Pattern * patterns = NULL;

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
      #if defined(__GNUC__)  &&  (__GNUC__ >= 7)
        __attribute__((fallthrough));
      #endif
    case 2: 
      k ^= key[1] << 8;
      #if defined(__GNUC__)  &&  (__GNUC__ >= 7)
        __attribute__((fallthrough));
      #endif
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
.. Create a new macroa nd corresponding pattern
*/
static int macro_new (char * macro, char * pattern) {
  #define COPY(start) do {                                           \
     size_t l = ptr - start;                                         \
     if (l + len >= RGXSIZE) {                                       \
       error ("rgx buffer out of memory");                           \
       return RGXERR;                                                \
     }                                                               \
     memcpy (& rgx [len], start, l);                                 \
     len += l;                                                       \
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
      /* {,n} {m,n} {m} */
      COPY (start);
      continue;
    }

    char * child = ptr - 1;
    if ( !(c == '_' || (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z')) )
      return RGXERR;

    do {
      /* Substitute macro by corresponding regex */
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

      error ("invalid name");
      return RGXERR;
    } while ( (c = *ptr++) );

    if (!c) {
      error ("incomplete }");
      return RGXERR;
    }
  }
  rgx [len] = '\0';
  int rpn [RGXSIZE];
  int status = rgx_rpn (rgx, rpn); /* check regex validity */
  if (status <= 0) {
    error ("cannot create rpn for regex %s in line %d", rgx, line);
    return RGXERR;
  }
  rgx [status] = '\0'; /* remove trailing white spaces */

  /* Insert macro to hashtable*/
  p = lookup (macro);
  if ( (m = *p) != NULL ) {
    error ( "macro %s redefined in lexer definition in line %d",
      macro, line);
    return RGXERR;
  }
  *p = m = allocate (sizeof (Macro));
  m->key = allocate_str (macro);
  m->rgx = allocate_str (rgx);

  if ( ! ( (rgx [0] == '[' && rgx [status-1] == ']') ||
       (rgx [0] == '(' && rgx [status-1] == ')') ) ) {
    error ("warning : macro %s recommended in ( )", macro);
  }
  printf ("\nmacro %s rgx %s", macro, rgx); fflush (stdout);
  
  return 0;
}

int lex_read_definitions ( FILE * in ) {

  char buff [PAGE_SIZE], *ptr, c;
  int code = 0, comment = 0;

  while (fgets (buff, sizeof buff, in)) { /*read line by line */
    line++;

    size_t len = strlen (buff);

    if ( buff [ len-1 ] != '\n' && !feof (in) ) {
      error ("very long line");
      return RGXOOM;
    }

    /*
    .. Source code section ^%{ ^%}
    */
    if (code) {
      if ( buff [0] == '%' && buff [1] == '}' ) {
        code = 0; continue;
      }
      Snippet * s = allocate ( sizeof (Snippet) );
      s->code = allocate_str (buff);
      *snippets = s;
      snippets = & s->next;
      continue;
    }

    ptr = buff; c = *ptr++;
    /*
    .. Consuming comment section, white spaces and empty lines
    */
    if ( c == '/' && !comment ) {
      if ( (c = *ptr++) != '*' ) {
        error ("unknown syntax");
        return RGXERR;
      }
      c = *ptr++; comment = 1;
    }
    if (comment || c == ' ' || c == '\t' || c == '\n') {
      do {
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

    /*
    .. End of definition or end of code snippet
    */
    if ( c == '%' ) {
      c = *ptr++;
      if (c == '{' ) {
        code = 1;
        continue;
      }
      if (c == '%')  /* %% is the end of definition section */
        return 0;
      /*
      .. Other % option, %top {}, etc .. not implemented
      */
      error ("warning : unknown/unimplemented in line %d", line);
      continue;
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
    .. Anything other than %**, macro definition and empty/comments
    .. are taken as unknown syntax
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
.. line. The regex will be stored in "buff".
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
        printf ("\npattern  %s", rgx); fflush (stdout);
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

static
int _action (FILE * fp, char * buff, size_t lim) {
  /* fixme : remove the limit. use realloc() to handle large buffer */

  int c, j = 0, depth = 0, quote = 0, escape = 0;
  /*
  .. Consume white spaces before encountering '{' i.e start of action.
  .. Warning : Action need to be in the same line as regex.
  */
  while ( (c = fgetc(fp)) != EOF ) {
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
  while ( (c = fgetc(fp)) != EOF ) {
    buff[j++] = (char) c;
    if (j == lim) {
      error ( "lxr grammar: action buff limit" );
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
    error ("lxr grammar : incomplete action. Missing '}'");
    return RGXERR;
  }

  while ( (c = fgetc(fp)) != EOF ) {
    if ( c == '\n') {
      line++;
      break;
    }
    if ( ! (c == ' ' || c == '\t' ) ) {
      error ("lxr grammar : unexpected character after action");
      return RGXERR;
    }
  }
  return 0;
}

typedef struct Action {
  char * rgx, * action;
  int line;
} Action;

static Stack * actions = NULL;

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
    if ( status == RGXERR ) return RGXERR;
    if ( status == NOACTION ) {
      pattern_action (rgx, "", cline);
      continue;
    }
    #undef EMPTY
    #undef NOACTION

    /* Read the action for the above regex pattern */
    status = _action (in, action, sizeof (action));
    if ( status == RGXERR ) return RGXERR;

    pattern_action (rgx, action, cline);
  }
  return 0;  
}

int lex_tables () {
  size_t n = ( actions->len / sizeof (void *) );
  char ** rgxs = allocate ( n * sizeof (char *) );
  Action ** A = (Action **) actions->stack;
  for (int i=0; i<n; ++i)
    rgxs [i] = A[i]->rgx;
}

int read_lex_input (FILE * fp) {
  line = 0;
  if (lex_read_definitions (fp) < 0) {
    error ("failed reading defintion section. line %d", line);
    return RGXERR;
  }
  if (lex_read_rules (fp) < 0) {
    error ("failed reading rules section. line %d", line);
    return RGXERR;
  }
  fprintf (stderr, "\nwarnings ");
  errors ();
  return 0;
}
