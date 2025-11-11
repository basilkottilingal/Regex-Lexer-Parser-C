  /*
  .. following set of patterns is not acceptable as
  .. one or more of the following patterns will never
  .. be matched. Ex : bc+ is a subset of bc* and
  .. bc* appears before bc+ in the list.
  */
%%
aa|b       { printf ("0"); }
a(a|b)     { printf ("1"); }
bc*        { printf ("2"); }
bc+        { printf ("3"); }
(a|b)+0    { printf ("5"); }
abc$       {}
abcd       {}
%%
