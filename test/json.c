/*
.. test case for nfa.
.. $ make test-nfa.s && ./test-nfa
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"
#include "nfa.h"

int main () {
  char * rgx[] = { 
    "true", "false", "null", 
    /*"[ \\t\\v\\n\\f]+",*/ "\\[", "\\]", ":", ",", "\\{", "\\}", 
    "-?(0|[1-9][0-9]*)(\\.[0-9]+)?([eE][+-]?[0-9]+)", 
    "\"([^\"\\\\]|\\\\[\"\\/bfnrt])*\"" /* fixme : allow unicode */
  };

  int nrgx = sizeof (rgx) / sizeof (rgx[0]);
  printf ("json regex patterns");
  for (int i=0; i<nrgx; ++i)
    printf ("\n%s", rgx[i]);
  DState * dfa = NULL;
  int status = rgx_list_dfa (rgx, nrgx, &dfa);
  if (status < 0) {
    errors ();
    printf ("cannot make DFA. aborting");
    exit (-1);
  }

  const char *json = "{\n"
    "    \"widget\": {\n"
    "        \"debug\": \"on\",\n"
    "        \"window\": {\n"
    "            \"title\": \"Sample Konfabulator Widget\",\n"
    "            \"name\": \"main_window\",\n"
    "            \"width\": 500,\n"
    "            \"height\": 500\n"
    "        },\n"
    "        \"image\": { \n"
    "            \"src\": \"Images/Sun.png\",\n"
    "            \"name\": \"sun1\",\n"
    "            \"hOffset\": 250,\n"
    "            \"vOffset\": 250,\n"
    "            \"alignment\": \"center\"\n"
    "        },\n"
    "        \"text\": {\n"
    "            \"data\": \"Click Here\",\n"
    "            \"size\": 36,\n"
    "            \"style\": \"bold\",\n"
    "            \"name\": \"text1\",\n"
    "            \"hOffset\": 250,\n"
    "            \"vOffset\": 100,\n"
    "            \"alignment\": \"center\",\n"
    "            \"onMouseUp\": \"sun1.opacity = (sun1.opacity / 100) * 90;\"\n"
    "        }\n"
    "    }\n"
    "}\n";
  char buff[64];
  printf ("\nparsing json text %s\n", json);
    
  const char * source = json;
  do {
    int m = rgx_dfa_match (dfa, source);
    if (m < 0) {
      printf ("internal error");
      errors ();
      exit (-1);
    }
    else if (m > 0) {
      m -= 1; buff[m] = '\0';
      if(m) memcpy (buff, source, m);
      printf ("\n%s", buff);
    }
    source += m < 1 ? 1 : m;
  } while (*source);

  /* free all memory blocks created */
  rgx_free();
}
