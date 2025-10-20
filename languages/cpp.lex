/**
.. c preprocessor tokenizer. Based on ISO C11 ( ISO/IEC 9899:2011 )
.. https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf
.. Note that the C11 preprocessor rules are a superset of C99's rules.
*/

WS_CPP (([ \t]|\\\n)*)
SP_CPP ((\\\n)*[ \t]([ \t]|\\\n)*)

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
WS     [ \t\v\n\f]
STRING (\"([^"\\\n]|{ES})*\")

%{
  
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>

  #include "tokens.h"
  
  FILE * out;
  static char file [1024];
  static void location  ( );
  static void header    ( int is_std );
  static int  env_if    ( );//char * expr );
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
%}
	 
%%
"/*"      {
            int c;
            while ( (c = lxr_input () ) != EOF ) {
              if ( c != '*' ) continue;
              while ( (c = lxr_input ()) == '*' ) {}
              if ( c == '/' ) break;
            }
          }
"//".*    { /* consumes */ }
^[ \t]*#{SP_CPP}[0-9]+{SP_CPP}{STRING}.*   {
            location ();
          }
^[ \t]*#{WS_CPP}"include"{WS_CPP}<[a-z_A-Z0-9/.\\+~-]+>.*    {
            header (1);
          }
^[ \t]*#{WS_CPP}"include"{WS_CPP}{STRING}.*   {
            header (0);
          }
^[ \t]*#{WS_CPP}"if"{SP_CPP}  { 
            env_if ();
          }
^[ \t]*#{WS_CPP}"ifdef"       { 
            env_if ();
          }
^[ \t]*#{WS_CPP}"ifndef"      { 
            env_if ();
          }
^[ \t]*#{WS_CPP}"elif"        {
            env_elif ();
          }
^[ \t]*#{WS_CPP}"else".*      {
            env_else ();
          }
^[ \t]*#{WS_CPP}"endif".*     {
            env_endif ();
          }
^[ \t]*#{WS_CPP}"define"      {
            cpp_error ("unknown # define %s", yytext);
          }
^[ \t]*#{WS_CPP}"define"{SP_CPP}{ID}   {
            /* shouldn't clash with kwywords */
            define_macro ();
          }
^[ \t]*#{WS_CPP}"undef"{SP_CPP}{ID}.*  {
            undefine_macro ();
          }
^[ \t]*#  {
            printf ("\n unknown # directive %s", yytext);
          }
{ID}      { 
            /* fixme : classify : keywords, id, enum, typedef*/
            /* see if it's a macro */
            printf ("\nid %s:", yytext);
            return IDENTIFIER; 
          }
{ID}{WS}*\(    { 
            /* see if it's a macro */
            printf ("\nfunc %s:", yytext);
          }
{STRING}                                             {
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

  /* ({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+   { return STRING_LITERAL; }*/

">>"					                       { return RIGHT_OP; }
"<<"					                       { return LEFT_OP; }
"&&"					                       { return AND_OP; }
"||"					                       { return OR_OP; }
"<="					                       { return LE_OP; }
">="					                       { return GE_OP; }
"=="					                       { return EQ_OP; }
"!="					                       { return NE_OP; }
"("|")"|"&"|"!"|"~"|"-"|"+"|"*"|"/"|"%"|"^"|"|"|"?" {
            /* single character tokens */
            return yytext [0];
          }

(\\\n)+                              { /* consume */ }    
(.|\n)			                         { /* echo */ }
	 
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
    char * s = strchr (yytext, '#') + 1;
    lxr_line_no = atoi (s) - 1;
    s = strchr (s, '"') + 1;
    snprintf (file, sizeof file, "%s", s);
    s = strchr (file, '"');
    if (s)
      *s = '\0';
    #if 0
    else
      cpp_error ("very long source name");
    #endif
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
  #define CPP_EXPR_CONSUME       32
  #define CPP_EXPR_EVALUATE      64
  #define CPP_EXPR_VALUE()        1
  #define CPP_EXPR_COMPLETE(val) do {                        \
      int * s = & env_status [env_depth];                    \
      if (!( (*s) & EXPR_EVALUATE) && !(val)) break;         \
      if ( CPP_EXPR_VALUE () ) {                             \
        (*s) |= CPP_PARSE_ON;                                \
        (*s) &= ~ (CPP_PARSE_NOTYET);                        \
        (*s)                                                 \ 
      }                                                      \
      (*s) &= ~ (CPP_EXPR_EVALUATE|CPP_EXPR_CONSUME);        \
    } while (0)                                              \

  static int expression_start () {
    int * status = & env_status [env_depth];
    if (*status & CPP_EXPR_EVALUATE) 
  }

  static int env_if ( ) {
    if (env_depth == CPP_ENV_DEPTHMAX) {
      cpp_error ("reached max depth of #if breanching");
      exit (-1);
    }
    env_status [++env_depth] = CPP_EXPR_EVALUATE; 
    printf ("\n#if%s", yytext);
  }

  static int env_elif ( ) {
    int * status = & env_status [env_depth];
    if ( ! (*status & CPP_IF) ) {
      cpp_error ("#elif without #if");
      exit (-1);
    }
    switch ( *status & 
      (CPP_PARSE_ON|CPP_PARSE_DONE|CPP_PARSE_NOTYET) ) 
    {
      case CPP_PARSE_ON : 
        *status &= ~CPP_PARSE_ON;
        *status |=  CPP_PARSE_DONE;
      case CPP_PARSE_DONE : 
        *status |= CPP_EXPR_CONSUME;
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
    if ( (*status) & CPP_IF ) {
      cpp_error ("#else without #if");
      exit (-1);
    }
    if ( *status & CPP_PARSE_NOTYET )
      *status |= (CPP_PARSE_ON|CPP_ELSE);
    *status &= ~CPP_IF;
  }

  static int env_endif ( ) {
    if ( (*status) & (CPP_IF|CPP_ELSE) ) {
      cpp_error ("#endif without #if");
      exit (-1);
    }
    --env_depth;
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
    Macro ** p = & table [h % table_size], * m;
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
      
    int tkn;
    while ( (tkn = lxr_lex()) ) {
      printf ("\n[%3d] : %s", tkn, yytext);
    }
  }
