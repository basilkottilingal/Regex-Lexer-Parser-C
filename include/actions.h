#ifndef _HASH_H_
#define _HASH_H_
  /*
  .. Read lxr grammar. and push
  .. (rgx, action) pair to ths stack
  */
  int lxr_actions ( const char * file, Stack * rgxList, Stack * actionList);
#endif
