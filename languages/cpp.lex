/**
.. c preprocessor tokenizer. Based on ISO C11 ( ISO/IEC 9899:2011 )
.. https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf
.. Note that the C11 preprocessor rules are a superset of C99's rules.
..
.. Important, assumes \\\n and comments are removed
*/

O      [0-7]
D      [0-9]
NZ     [1-9]
L      [a-zA-Z_]
A      [a-zA-Z_0-9]
H      [a-fA-F0-9]
ID     ([_a-zA-Z][_a-zA-Z0-9]*)
HP     (0[xX])
E      ([Ee][+-]?{D}+)
P      ([Pp][+-]?{D}+)
FS     (f|F|l|L)
IS     (((u|U)(l|L|ll|LL)?)|((l|L|ll|LL)(u|U)?))
CP     (u|U|L)
SP     (u8|u|U|L)
ES     (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
WS1    [ \t]
WS     [ \t\v\n\f]
STRING (\"([^"\\\n]|{ES})*\")

%{
  
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <stdint.h>

  #include "tokens.h"
  
  #define CPP_NONE               0
  #define CPP_IF                 1
  #define CPP_ELSE               2
  #define CPP_PARSE_ON           4
  #define CPP_PARSE_DONE         8
  #define CPP_PARSE_NOTYET       16
  #define CPP_EXPR_EVALUATE      64
  #define CPP_DEFINE             128
  #define CPP_ENV_DEPTHMAX       63
  #define CPP_ISNEGLECT                                              \
    if ( !(*status & (CPP_PARSE_ON|CPP_EXPR_EVALUATE)) )             \
      break

  static int * status;
  static FILE * out;
  static int  verbose;

  static char file [1024];
  static void location  ( );
  static void header    ( int is_std );
  static int  env_if    ( );
  static int  env_elif  ( );
  static int  env_else  ( );
  static int  env_endif ( );
  static char * get_content ( );
  static void define_macro ( );
  static void undefine_macro ( );
  static void echo      ( char * str );
  static void warn_trailing ( );
  static void verbose_level (int);
  
  /*
  .. Error reporting
  */
  #define cpp_error(...)  do {                                       \
      fprintf(stderr, "\nerror : file %s line %d :",                 \
        file, lxr_line_no );                                         \
      fprintf(stderr, ##__VA_ARGS__);                                \
    } while (0)
  #define cpp_warning(...)  do {                                     \
      fprintf(stderr, "\nwarning : file %s line %d :",               \
        file, lxr_line_no );                                         \
      fprintf(stderr, ##__VA_ARGS__);                                \
    } while (0)
  #define cpp_fatal(...)  do {                                       \
      fprintf(stderr, "\nfatal error : file %s line %d :",           \
        file, lxr_line_no );                                         \
      fprintf(stderr, ##__VA_ARGS__);                                \
      exit(-1);                                                      \
    } while (0)
%}
	 
%%
(\\\n)     { 
            //cpp_error ("cpp internal error : slicing unexpected");
           }
"%:"{1,2}  { 
            //cpp_error ("cpp internal error : digraph %: expected");
          }
"//".*    { }
"/*"      {
            int c;
            while ( (c = lxr_input () ) != EOF ) {
              if ( c != '*' ) continue;
              while ( ( c = lxr_input () ) == '*' ) {}
              if ( c == '/' ) break;
            }
          }
^{WS1}*#({WS1}*"line")?{WS1}+[0-9]+{WS1}+{STRING}        {
            CPP_ISNEGLECT;
            location ();
            warn_trailing ();
          }
^{WS1}*#{WS1}*"include"{WS1}*<[a-z_A-Z0-9/.\\+~-]+>    {
            CPP_ISNEGLECT;
            header (1);
            warn_trailing ();
          }
^{WS1}*#{WS1}*"include"{WS1}*{STRING}   {
            CPP_ISNEGLECT;
            header (0);
            warn_trailing ();
          }
^{WS1}*#{WS1}*"if"{WS1}+       { 
            env_if ();
          }
^{WS1}*#{WS1}*"ifdef"{WS1}+    { 
            env_if (); 
          }
^{WS1}*#{WS1}*"ifndef"{WS1}+   { 
            env_if ();
          }
^{WS1}*#{WS1}*"elif"{WS1}+     {
            env_elif ();
          }
^{WS1}*#{WS1}*"else"           {
            env_else ();
            warn_trailing ();
          }
^{WS1}*#{WS1}*"endif"          {
            env_endif ();
            warn_trailing ();
          }
^{WS1}*#{WS1}*"define"{WS1}+{ID}   {
            CPP_ISNEGLECT;
            define_macro ();
          }
^{WS1}*#{WS1}*"undef"{WS1}+{ID}  {
            CPP_ISNEGLECT;
            undefine_macro ();
            warn_trailing ();
          }
^{WS1}*#  {
            cpp_error ("\n unknown # directive %s", yytext);
          }
{ID}      { 
            return IDENTIFIER; 
          }
{STRING}  {
            /* ({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+   { 
                return STRING_LITERAL; }
              fixme : may concatenate any consecutive string*/
            return STRING_LITERAL;
          }

{HP}{H}+{IS}?				                 { return I_CONSTANT; }
{NZ}{D}*{IS}?				                 { return I_CONSTANT; }
"0"{O}*{IS}?				                 { return I_CONSTANT; }
{CP}?"'"([^'\\\n]|{ES})+"'"		       { return I_CONSTANT; }

{D}+{E}{FS}?				                 { return F_CONSTANT; }
{D}*"."{D}+{E}?{FS}?			           { return F_CONSTANT; }
{D}+"."{E}?{FS}?			               { return F_CONSTANT; }
{HP}{H}+{P}{FS}?			               { return F_CONSTANT; }
{HP}{H}*"."{H}+{P}{FS}?			         { return F_CONSTANT; }
{HP}{H}+"."{P}{FS}?			             { return F_CONSTANT; }

">>"					                       { return RIGHT_OP; }
"<<"					                       { return LEFT_OP; }
"&&"					                       { return AND_OP; }
"||"					                       { return OR_OP; }
"<="					                       { return LE_OP; }
">="					                       { return GE_OP; }
"=="					                       { return EQ_OP; }
"!="					                       { return NE_OP; }

(.|\n)                               { return yytext[0]; }
	 
%%

static void verbose_level (int l) {
  verbose = l;
}
   
static char * get_content () {
  int c, p = '\0', len = yyleng;
  while ( (c = lxr_input () ) != EOF ) {
    if ( c == '\n' && p != '\\') break;
    p = c;
  }
  lxr_token ();
  return & yytext [len];
}

static void warn_trailing () {
  int err = 0, c, p = '\0';
  while ( (c = lxr_input ()) != '\0' ) {
    if ( c == '/' && p == '/' ) {
      --err;
      while ( (c = lxr_input ()) != '\0' && c != '\n' ) {
      }
      break;
    }
    if ( c == '\n' ) {
      if ( p != '\\' ) break;
      --err;
    }
    else if ( c == '*' && p == '/' ) {
      --err;
      while ( (c = lxr_input ()) != EOF ) {
        if ( c != '*' ) continue;
        while ( ( c = lxr_input () ) == '*' ) {
        }
        if ( c == '/' ) break;
      }
      if ( c == EOF )
        cpp_warning ("non-terminated /*");
    }
    else if ( ! (c == ' ' || c == '\t') )
      ++err;
    p = c; 
  }
  if (err)
    cpp_warning ("\nbad trailing characters after %s\n", yytext);
}

static
void echo ( char * str ) {
  fprintf (out, "%s", str);
}

static
void echo_line () {
  fprintf (out, "\n# %d \"%s\"\n", lxr_line_no, file);
}

static
void location () {
  char * s  = strchr (yytext, '"'); *s = '\0';
  char * _s = strchr (yytext, 'e'); *s++ = '"';
  lxr_line_no = atoi (_s ? _s+1 : strchr(yytext, '#') + 1) - 1;
  snprintf (file, sizeof file, "%s", s);
  s = strchr (file, '"');
  if (s) *s = '\0';
  else cpp_warning ("very long source name");
  printf ("\n loc # %d \"%s\"", lxr_line_no, file);
}

static
void header (int is_std) {
  if (is_std) {
    printf ("\n std header %s", yytext);
    return; 
  }
  printf ("\nuser defined header %s", yytext);
}

static
int env_status [CPP_ENV_DEPTHMAX + 1] = {CPP_NONE|CPP_PARSE_ON};
static int env_depth = 0;                     /* fixme : may resize */

static int env_if ( ) {
  if (env_depth == CPP_ENV_DEPTHMAX) {
    cpp_fatal ("reached max depth of #if breanching");
    exit (-1);
  }
  ++env_depth;
  *++status = CPP_EXPR_EVALUATE|CPP_IF;
  return 0;
}

static int env_elif ( ) {
  if ( ! (*status & CPP_IF) ) {
    cpp_warning ("#elif without #if");
    exit (-1);
  }
  switch (*status & (CPP_PARSE_ON|CPP_PARSE_NOTYET|CPP_PARSE_DONE))
  {
    case CPP_PARSE_DONE :
      break; 
    case CPP_PARSE_ON : 
      *status &= ~CPP_PARSE_ON;
      *status |=  CPP_PARSE_DONE;
      break;
    case CPP_PARSE_NOTYET :
      *status |= CPP_EXPR_EVALUATE;
      break;
    default :
      cpp_fatal ("cpp - internal error : invalid parse status");
      break;
  }
  return 0;
}

static int env_else ( ) {
  if ( !((*status) & CPP_IF) ) {
    cpp_warning ("#else without #if");
  }
  *status &= ~(CPP_IF|CPP_PARSE_ON);
  *status |=  CPP_ELSE;
  if ( *status & CPP_PARSE_NOTYET )
    *status |= CPP_PARSE_ON;
  return 0;
}

static int env_endif ( ) {
  if ( !((*status) & (CPP_IF|CPP_ELSE)) ) {
    cpp_error ("#endif without #if");
    exit (-1);
  }
  --env_depth;
  --status;
  return 0;
}

typedef struct Macro Macro;
static Macro ** macro_lookup ( const char * id );
static char *   macro_substitute ( Macro * m, char ** args );
typedef struct Macro {
  char * key, * val, ** args;
  struct Macro * next;
  int  tu_scope;                                /* translation unit */
} Macro;

static Macro ** macros;
static Macro * freelist = NULL;   /* hashtable & freelist of macros */
#ifndef CPP_MACRO_TABLE_SIZE
#define CPP_MACRO_TABLE_SIZE 8
#endif
int table_size = CPP_MACRO_TABLE_SIZE;

static
Macro * macro_allocate () {
  if (!freelist) {
    Macro * m = freelist =
      malloc (CPP_MACRO_TABLE_SIZE * sizeof (Macro));
    if (!m)
      cpp_fatal ("dynamic memory alloc failed in macro_allocate()");
    for (int i=0; i<CPP_MACRO_TABLE_SIZE-1; ++i) {
      *((Macro **) m) = m+1;
      ++m;
    }
    *((Macro **) m) = NULL;
  }
  Macro * m = freelist;
  freelist = *((Macro **) m);
  memset (m, 0, sizeof (Macro));
  return m;
}

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
  Macro ** p = & macros [h % table_size], * m;
  while ( (m = *p) != NULL ) {
    if ( !strcmp (m->key, key) )
      return p;
    p = & m->next;
  }
  return p;
}

static void define_macro () {
  char * content = get_content ();
  printf ("\n#define %s : isfunc %d", yytext, *content == '(' );
}

static void undefine_macro () {
  printf ("\n#undef %s", yytext);
}

typedef struct Token {
  char * start, * end;
  int token;
} Token;

static int expr_eval ( Token * expr, int n ) {
  char holdchar, * start, * end;
  printf ("\neval : ");
  while (n--) {
    start = expr->start;
    end = expr->end;
    holdchar = *end;
    *end = '\0';
    printf ("%s ", start);
    *end = holdchar;
    ++expr;
  }
  printf ("\n");
  /* fixme : implement top-to-bottom LL parser to evaluate expr */
  return 1;
}

int main ( int argc, char * argv[] ) {
  /*
  .. In case you want to overload gcc macros, run this gcc command
  .. via posix popen()
  .. FILE *fp = popen("gcc -dM -E - < /dev/null", "r");
  .. 
  */
  out = stdout;
  table_size = 1 << 8;
  macros = malloc (table_size * sizeof (Macro *)); 
  if (!macros)
    cpp_fatal ("dynamic allocation for macros hashtable failed ");
  memset (macros, 0, table_size * sizeof (Macro *));
  status = env_status;
  
  if (argc > 1)
    lxr_source (argv[1]);
  snprintf (file, sizeof file, "%s",
    (argc > 1) ? argv [1] : "<stdin>");
    
  int tkn, nexpr = 128, iexpr = 0;
  Token * expr = malloc (nexpr * sizeof (Token));
  while ( (tkn = lxr_lex()) ) {
    switch ( *status & (CPP_PARSE_ON|CPP_EXPR_EVALUATE) ) {
      case 0 :
        break;
      case CPP_PARSE_ON :
        printf ("%s", yytext);
        break;
      case CPP_EXPR_EVALUATE :
        if (tkn == '\n') {
          *status &= ~CPP_EXPR_EVALUATE;
          if ( expr_eval (expr, iexpr) ) {
            *status |=  CPP_PARSE_ON;
            *status &= ~CPP_PARSE_NOTYET;
          }
          iexpr = 0;
          break;
        }
        if (tkn == ' ' || tkn == '\t')
          break;
        if ( iexpr == nexpr )
          expr = realloc (expr, (nexpr *= 2)*sizeof (Token));
        expr[iexpr++] =    /* stack tokens to evaluate expr later */
          (Token) {
            .start = yytext,
            .end   = yytext + yyleng,
            .token = tkn
          };
        break;
      default :
        cpp_fatal ("cpp internal error : unknown parse status"); 
    }
  }

  free (expr);
  free (macros);

  if (status != env_status)
    cpp_warning ("# if not closed");

  return 0;
}
