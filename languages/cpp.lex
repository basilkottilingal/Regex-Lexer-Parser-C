/**
.. c preprocessor tokenizer. Based on ISO C11 ( ISO/IEC 9899:2011 )
.. https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf
.. Note that the C11 preprocessor rules are a superset of C99's rules.
*/

WS      ([ \t]|\\\n)
SP      ((\\\n)*[ \t]([ \t]|\\\n)*)
ID      ([_a-zA-Z][_a-zA-Z0-9]*)
ES      (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
STRING  \"([^"\\\n]|{ES})*\"

%{
  
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  
  static char * file = NULL;
  static void location  ( );
  static void header    ( int is_std );
  static int  env_if    ( char * expr );
  static int  env_elif  ( char * expr );
  static int  env_else  ( );
  static void env_endif ( );
  static char * get_content ();
  static void define_macro ( );
  static void undefine_macro ( );
  
  /*
  .. Error reporting
  */
  #define cpp_error(...)  do {                                         \
      fprintf(stderr, "file %s line %d :", file, lxr_line_no );        \
      fprintf(stderr, "cpp error", __VA_ARGS__);                       \
      exit(-1);                                                        \
    } while (0)
%}
	 
%%

"/*"                                                   {
            int c;
            while ( (c = lxr_input () ) != EOF ) {
              if ( c != '*' ) continue;
              while ( (c = lxr_input ()) == '*' ) {}
              if ( c == '/' ) break;
            }
          }
"//".*                                                 { }
^[ \t]*#{SP}[0-9]+{SP}{STRING}.*           {
            location ();
          }
^[ \t]*#{WS}*"include"{WS}*<[a-z_A-Z0-9/.\\+~-]+>.*    {
            header (1);
          }
^[ \t]*#{WS}*"include"{WS}*{STRING}.*                  {
            header (0);
          }
^[ \t]*#{WS}*"if"                                      { 
            env_if ( get_content ());
          }
^[ \t]*#{WS}*"elif"                                    {
            env_elif ( get_content ());
          }
^[ \t]*#{WS}*"else".*                                  {
            env_else ( );
          }
^[ \t]*#{WS}*"endif".*                                 {
            env_endif ( );
          }
^[ \t]*#{WS}*"define"                                  {
            get_content ();
            cpp_error ("unknown # define", yytext);
          }
^[ \t]*#{WS}*"define"{SP}{ID}                          {
            /* shouldn't clash with kwywords */
            define_macro ();
          }
^[ \t]*#{WS}*"undef"{SP}{ID}.*                   {
            undefine_macro ();
          }
^[ \t]*#	                                             {
            get_content ();
            printf ("\n unknown # directive %s", yytext);
          }
{ID}                                      { /* see if it's a macro */
            printf ("\nid %s:", yytext);
          }
{ID}[ \t\n]*\(                            { /* see if it's a macro */
            printf ("\nfunc %s:", yytext);
          }
{STRING}                                  {
          }
(\\\n)+                                   { /* consume */ }    
(.|\n)			                              { /* echo */ }
	 
%%
     
  
  static char * get_content () {
    int c, p = '\0', len = yyleng;
    while ( (c = lxr_input () ) != EOF ) {
      if ( c == '\n' && p != '\\') break;
      p = c;
    }
    lxr_tokenize ();
    return & yytext [len];
  }
  
  static void file_macro () {
  }
  
  static void line_macro () {
  }
  
  static void time_macro () {
  }
  
  /*
  .. Note __func__ is not a macro like __FILE__, __LINE__ or __TIME__
  */
  
  static
  void location () {
    char * s = strchr (yytext, '#') + 1;
    lxr_line_no = atoi (s) - 1;
    s = strchr (s, '"');
    free (file);
    file = strdup (s);
    s = strchr (s, '"');
    *s = '\0';
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
 
  #if 0 
  typedef enum {
    IF,
    ELSE,
    DEFINE,
    NONE
  } STATUS ;
  
  typedef struct Env {
    STATUS status;
    struct Env * prev;
  } Env;
  
  Env env_root = (Env) { NONE, NULL };
  Env * env_head = & env_root;
  
  typedef struct Macro Macro;
  static Macro ** macro_lookup ( const char * id );
  static char *   macro_substitute ( Macro * m, char ** args );
  typedef struct Macro {
    char * key, * val, ** args;
    struct Macro * next;
    int  tu_scope;                   /* translation unit */
  } Macro;
  
  Macro ** macros;                          /* hashtable */
  #endif

  static void define_macro () {
    char * content = get_content ();
    printf ("\n#define %s : isfunc %d", yytext, *content == '(' );
  }

  static void undefine_macro () {
    printf ("\n#undef %s", id);
  }

  static int env_if ( char * expr ) {
    printf ("\n#if%s", expr);
  }

  static int env_elif ( char * expr ) {
    printf ("\n#elif%s", expr);
  }

  static int env_else ( ) {
    printf ("\n#else");
  }

  static int env_endif ( ) {
    printf ("\n#endif");
  }
  
  int main ( int argc, char * argv[] ) {
    /*
    .. In case you want to overload gcc macros, run this gcc command via
    .. posix popen()
    .. FILE *fp = popen("gcc -dM -E - < /dev/null", "r");
    .. 
    */
    
    if (argc > 1)
      lxr_source (argv[1]);
    char * file = strdup (( argc == 1 ) ? "<stdin>"  : argv [1]);
  
    int tkn;
    while ( (tkn = lxr_lex()) ) {
      printf ("\n[%3d] : %s", tkn, yytext);
    }
  }
