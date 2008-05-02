%option yylineno

%{
#include "parser.h"
#include "y.tab.h"

#if 0
#define RETURN(x) printf("TOK:%s\n",#x);  return(x)
#else
#define RETURN return
#endif

long strtol();
%}  

IDCHAR		[a-zA-Z0-9_\-\.]
NUMCHAR		[0-9]
S		[ \t\n\r\f]+
NAME		[a-zA-Z]{IDCHAR}*
NUM		{NUMCHAR}+
HEXNUM          "0x"[0-9A-Za-z]+
COMMENT		"/*"("*"?[^/]+)*("*/")
CODE            "{@"[^@]+"@}"
%%

".."			{RETURN(DOT_DOT_);}
"enum"			{RETURN(ENUM_);}
"struct"		{RETURN(STRUCT_);}
"COMMENTEDopaque"		{mkname();RETURN(OPAQUE_);}
"select"		{RETURN(SELECT_);}
"constant"              {RETURN(CONSTANT_);}
"digitally-signed"	{/* Do nothing*/}
"public-key-encrypted"  {/* Do nothing*/}
"case"			{RETURN(CASE_);}
"primitive"		{RETURN(PRIMITIVE_);}
"public"		{RETURN(PUBLIC_);}
"typedef"		{RETURN(TYPEDEF_);}

{COMMENT}		{ /*Do nothing*/}
{CODE}          { strcpy(yylval.str,yytext); RETURN(CODE_);}

{S}			{/*do nothing*/} 
{NAME}			{mkname();RETURN(NAME_);}
{NUM}			{yylval.val=atoi(yytext); RETURN(NUM_);}
{HEXNUM}			{yylval.val=strtol(yytext,0,16); RETURN(NUM_);}
"{"                      { RETURN('{'); }
"}"                      { RETURN('}'); }
"("                      { RETURN('('); }
")"                      { RETURN(')'); }
"["                      { RETURN('['); }
"]"                      { RETURN(']'); }
"<"                      { RETURN('<'); }
">"			 { RETURN('>'); }
";"                      { RETURN(';'); }
":"			 { RETURN(':'); }
","			 { RETURN(','); }
"."			 { RETURN('.'); }
"^"			 { RETURN('^'); }
"-"			 { RETURN('-'); }
"="                      { RETURN('='); }
.			 { fprintf(stderr,"Invalid input token: %s at %d!!!\n",yytext,yylineno);
			   exit(1);
			   }

%%
int mkname()
{
#if 0
printf("%s\n",yytext);
#endif
strcpy(yylval.str,yytext);
}

