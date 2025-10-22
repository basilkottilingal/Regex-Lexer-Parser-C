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
  
  FILE * out;
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
  
  /*
  .. Error reporting
  */
  #define cpp_error(...)  do {                                       \
      fprintf(stderr, "file %s line %d :", file, lxr_line_no );      \
      fprintf(stderr, ##__VA_ARGS__);                                \
      exit(-1);                                                      \
    } while (0)
  #define cpp_warning(...)  do {                                     \
      fprintf(stderr, "file %s line %d :", file, lxr_line_no );      \
      fprintf(stderr, ##__VA_ARGS__);                                \
    } while (0)
%}
	 
%%
(\\\n)    { 
            /*cpp_warning ("cpp internal error : \\\\\\n"
              "expected to be removed before preprocessing");*/
          }
"%:"      { 
            /*cpp_error ("cpp internal error : %s"
              "expected to be replaced by '#'", yytext); */
          }
"//".*    {
            /*cpp_warning ("cpp internal error : inline"
              "comment is expected to be removed apriori"); */
          }
"/*"      {
            int c;
            while ( (c = lxr_input () ) != EOF ) {
              if ( c != '*' ) continue;
              while ( ( c = lxr_input () ) == '*' ) {}
              if ( c == '/' ) break;
            }
            /*cpp_warning ("cpp internal error : inline"
              "comment is expected to be removed apriori"); */
          }
^{WS1}*#({WS1}*"line")?{WS1}+[0-9]+{WS1}+{STRING}.*   {
            /* fixme : warn any trailing characters */
            location ();
          }
^{WS1}*#{WS1}*"include"{WS1}*<[a-z_A-Z0-9/.\\+~-]+>.*    {
            /* fixme : warn any trailing characters */
            header (1);
          }
^{WS1}*#{WS1}*"include"{WS1}*{STRING}.*   {
            /* fixme : warn any trailing characters */
            header (0);
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
^{WS1}*#{WS1}*"else".*         {
            /* fixme : warn any trailing characters */
            env_else ();
          }
^{WS1}*#{WS1}*"endif".*       {
            /* fixme : warn any trailing characters */
            env_endif ();
          }
^{WS1}*#{WS1}*"define"{WS1}+{ID}   {
            /* shouldn't clash with kwywords */
            define_macro ();
          }
^{WS1}*#{WS1}*"undef"{WS1}+{ID}.*  {
            /* fixme : warn any trailing characters */
            undefine_macro ();
          }
^{WS1}*#  {
            cpp_error ("\n unknown # directive %s", yytext);
          }
{ID}      { 
            /* fixme : classify : keywords, id, enum, typedef*/
            /* see if it's a macro */
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
     
  
  static char * get_content () {
    int c, p = '\0', len = yyleng;
    while ( (c = lxr_input () ) != EOF ) {
      if ( c == '\n' && p != '\\') break;
      p = c;
    }
    lxr_token ();
    return & yytext [len];
  }
  
  
  /*
  .. Note __func__ is not a macro like __FILE__, __LINE__ or __TIME__
  */

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
    char * s = strchr (yytext, '"'); *s++ = '\0';
    char * _s = strchr (yytext, 'e');
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
 

  #define CPP_NONE               0
  #define CPP_IF                 1
  #define CPP_ELSE               2
  #define CPP_ENV_DEPTHMAX       63
  static int env_status [CPP_ENV_DEPTHMAX + 1] = {CPP_NONE};
  static int env_depth = 0;
  #define CPP_PARSE_ON           4
  #define CPP_PARSE_DONE         8
  #define CPP_PARSE_NOTYET       16
  #define CPP_EXPR_EVALUATE      64


  static int env_if ( ) {
    if (env_depth == CPP_ENV_DEPTHMAX) {
      cpp_error ("reached max depth of #if breanching");
      exit (-1);
    }
    env_status [++env_depth] = CPP_EXPR_EVALUATE|CPP_IF; 
    return 0;
  }

  static int env_elif ( ) {
    int * status = & env_status [env_depth];
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
        cpp_error ("cpp - internal error : invalid parse status");
        break;
    }
    return 0;
  }

  static int env_else ( ) {
    int * status = & env_status [env_depth];
    if ( !((*status) & CPP_IF) ) {
      cpp_warning ("#else without #if");
    }
    *status &= ~CPP_IF;
    *status |=  CPP_ELSE;
    if ( *status & CPP_PARSE_NOTYET ) {
      *status |= CPP_PARSE_ON;
    }
    return 0;
  }

  static int env_endif ( ) {
    int * status = & env_status [env_depth];
    if ( !((*status) & (CPP_IF|CPP_ELSE)) ) {
      cpp_error ("#endif without #if");
      exit (-1);
    }
    --env_depth;
    return 0;
  }
  
  typedef struct Macro Macro;
  static Macro ** macro_lookup ( const char * id );
  static char *   macro_substitute ( Macro * m, char ** args );
  typedef struct Macro {
    char * key, * val, ** args;
    struct Macro * next;
    int  tu_scope;                   /* translation unit */
  } Macro;
  
  Macro ** macros;                          /* hashtable */
  int table_size;
  
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

  int expr_eval ( Token * expr, int n ) {
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
    macros = malloc ( table_size * sizeof (Macro *) );
    memset (macros, 0, table_size * sizeof (Macro *)); 
    
    if (argc > 1)
      lxr_source (argv[1]);
    snprintf (file, sizeof file, "%s",
      (argc > 1) ? argv [1] : "<stdin>");
      
    int tkn, nexpr = 128, iexpr = 0;
    Token * expr = malloc (nexpr * sizeof (Token));
    while ( (tkn = lxr_lex()) ) {
      int * status = & env_status [env_depth];
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
          cpp_error ("cpp internal error : unknown parse status"); 
      }
    }
    //free (expr);
    free (macros);
  }
