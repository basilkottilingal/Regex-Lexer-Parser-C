  /*
  .. Invalid set of patterns. There is a pattern with zero length.
  */
%%
aa|b       { printf ("0"); }
a(a|b)     { printf ("1"); }
c*        { printf ("2"); }
bc+        { printf ("3"); }
(a|b)+0    { printf ("5"); }
abc$       {}
abcd       {}
%%
