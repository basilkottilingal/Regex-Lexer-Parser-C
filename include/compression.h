#ifndef _ROW_CMP_H
#define _ROW_CMP_H

  typedef struct Row {
    int s, n, start, span, token, empty;
    int * stack;
  } Row;

  int rows_compression (Row **, int ***, int **, int n, int m);

#endif
