#ifndef _LXR_H_
#define _LXR_H_

  #include "stack.h"

  /* 
  .. Available guaranteed buffersize.
  */
  #ifndef LXR_BUFFSIZE
  #define LXR_BUFFSIZE 256
  #endif

  /* 
  .. input source is either a file / string 
  */
  int lxr_source   ( const char * file );
  int lxr_string   ( const char * txt );

  /* 
  .. The main iterator function. 
  .. May pass a null if you don't need the token
  .. string
  */
  int lxr_tokenize ( char ** tkn );

  /* Read the lexer grammar file that has a list of pair of regex,
  .. and corresponding action ( once a token that matches a regex is
  .. found ).
  .. NOTE : 
  .. (a) Lexer will seek for the longest token that match atleast
  ..     one of the regex.
  .. (b) In case of regex collision, i.e the longest acceptable 
  ..     token satisfy multiple regex, the first action in the grammar
  ..     file will be executed
  .. (c) In grammar file, in each line regex should be followed by
  ..     by action inside the block { }
  .. (d) Empty lines are allowed. 
  */
  int lxr_grammar  ( const char * file, Stack * rgxs, Stack * actions);
  int lxr_generator  ( const char * file, const char * output);

  /*
  .. read a char from source. May use to consume
  */
  int lxr_input   ( );

#endif
