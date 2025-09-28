aa|b       { printf ("0"); }
a(a|b)     { printf ("1"); }
bc+        { printf ("3"); }
(a|b)+0    { printf ("5"); }
abc$       {}
^abc       {}
abcd       {}
