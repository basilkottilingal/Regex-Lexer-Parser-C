#ifndef _CLASS_H_
#define _CLASS_H_

  void class_init   ( );
  void class_get    ( int ** class, int * nclass );
  void class_refine ( int * list, int n );
  void class_char   ( int c );

  /*
  .. 3 equivalence classes are reserved for BOL (beginning of line) ,
  .. EOL (end of line) and EOB (end of buffer)
  */

  #define BOL_CLASS  (nclass - 1)
  #define EOL_CLASS  (nclass - 2)
  #define EOB_CLASS  (nclass - 3)

#endif
