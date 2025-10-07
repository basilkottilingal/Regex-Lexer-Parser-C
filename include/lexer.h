#ifndef _LXR_H_
#define _LXR_H_

  /* 
  .. Available guaranteed buffersize.
  */
  #ifndef LXR_BUFFSIZE
  #define LXR_BUFFSIZE 256
  #endif

  /* 
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
  int lxr_generate (FILE * in, FILE * out);
  int read_lex_input (FILE * in, FILE * out);

#endif
