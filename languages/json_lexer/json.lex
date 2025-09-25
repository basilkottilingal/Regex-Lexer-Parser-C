true                                            { TRUE }
false                                           { FALSE }
null                                            { NULL  }
[ \t\v\n\f]+                                    { WHITESPACE }
\[                                              { ARRAY_START }
\]                                              { ARRAY_END }
:                                               { COLON }
,                                               { COMMA }
\{                                              { OBJECT_START }
\}                                              { OBJECT_END }
-?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?  { NUMBER }
"([^"\\]|\\["\\/bfnrt])*"                       { STRING }
