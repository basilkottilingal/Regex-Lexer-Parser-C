int #
#else
  char assa[] = "inisde #else withot #if";
#endif
#define A 200 /* ...

*/
# if 0
  char str [] = "inside #if 0"; 
#else
int sss=0;
#endif
char aaaa = "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss";


#if A > 100 || /*
.. */ A < 233
int a = A;
#define func2 100
#else  ,,,, /* trailing warning*/
#endif

int main () {
  int b = 10|\
|8;
}

#line 23 "mine.j"
# 34 "<stdin>" /* {1,1,} */


#define float fff /* this is fine. you can redefine keywords */

float num = 10;

  /* redefinition without undef are warning */
#define func1(a) a+100
#undef func1
#define func1(b) b+100

#define func2 "aa"

#define func3 100
#define func3 101






#define macro(1) 11 /* wrong */
#define macro2(a,  foooo a+fooo /* wrong */
#define macro3(_a, {askmas}) _a /* wrong */
#define macro(a, /*fine */b) a+b
#define macro1(a, b) macro(1,2)+a+b

int macrosub = macro(macro1(3,4),5);  /* you might expect a catastrophy of looping of macro(), 
but it never happens. because macro1 has already expanded during it's definition,
and macro1(a,b) will look like a+b+a+b*/

int macrosub2 = macro(3+4, (int) 123u);

# /* Null directive */

#define str(a) char a = #a
#define strs(a,b) char a = #b

str(b);

str(); //right usage
strs(a,);
strs(,b);

strs();  /* wrong usage. insuff number of args*/

#define JOIN(a,b)  b ## a
#define JOIN2(a,b)  a ## b

int JOIN(my,int) = 25;

int JOIN2(my,) = 23; //valid
int JOIN2(,nmy) = 23; //valid
int JOIN() = 55;  //invalid
int JOIN(,) = 55;  //invalid

#define _FUNC_ #a

#define _FF_(b) b _FUNC_
int a =_FF_(1);

char a = 9 ._FUNC_;  /* '.' at the beginning doesn't stop from lexing the idenitifier _FUNC_ */
char a = 9_FUNC_;  /* But '9' at the beginning does stop from lexing the idenitifier _FUNC_ */
char a = "_FUNC_";

#define WW(a,b,c)a+b+c  /* can ')' separate args & defn ? Yes. This is valid*/

int ww = WW(1,1,1);

#define repeated(a,a) a*a //repeated arg. error

int rr = repeated (1,1);   

int/*...*/innnnt = 1;  /* don't forget to put a ' ' when you remove multiline comments */

char ut[] ="unterminated string can cause problem for tokens in other lines
#define __a(i)  i = 
char ut[] = 'another untrminated string with invalid character \t	

/* misplaced line splicing */\   jsbakjsa 
\     
char ww[] = "above line has white space after '\\'"; 

int c \   
= 1;

char c [] = "split \ 
quote"
