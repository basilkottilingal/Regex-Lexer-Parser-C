#ifndef _ROW_CMP_H
#define _ROW_CMP_H

  typedef struct Row {
    int s, n, token;
    uint32_t hash;
    int * stack;
  } Row;
  int rows_compression (Row **, int ***, int **, int n, int m);

#endif
