#ifndef _CLASS_H_
#define _CLASS_H_

  void class_init   ( );
  void class_get    ( int ** class, int * nclass );
  void class_refine ( int * list, int n );
  void class_char   ( int c );

  /*
  .. two equivalence classes are reserved for beginning of line,
  .. and end of line
  */

  #define BOL_CLASS  (nclass - 2)
  #define EOL_CLASS  (nclass - 1)

#endif
