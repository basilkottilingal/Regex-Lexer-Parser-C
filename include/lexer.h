#ifndef _LXR_H_
#define _LXR_H_

  /* 
  .. Available guaranteed buffersize.
  */
  #ifndef LXR_BUFFSIZE
  #define LXR_BUFFSIZE 256
  #endif

  /*
  .. Puts a limit to the token size limit. 
  */
  #ifndef LXR_TKNSIZE
  #define LXR_TKNSIZE 128
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

  /* Define lexer grammar using regex and action */
  int lxr_grammar  ( const char * file );

  /*
  .. read a char from source. May use to consume
  */
  int lxr_input   ( );

#endif
