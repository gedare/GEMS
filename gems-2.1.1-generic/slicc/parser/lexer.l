
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the SLICC (Specification Language for 
    Implementing Cache Coherence), a component of the Multifacet GEMS 
    (General Execution-driven Multiprocessor Simulator) software 
    toolset originally developed at the University of Wisconsin-Madison.

    SLICC was originally developed by Milo Martin with substantial
    contributions from Daniel Sorin.

    Substantial further development of Multifacet GEMS at the
    University of Wisconsin was performed by Alaa Alameldeen, Brad
    Beckmann, Jayaram Bobba, Ross Dickson, Dan Gibson, Pacia Harper,
    Derek Hower, Milo Martin, Michael Marty, Carl Mauer, Michelle Moravan,
    Kevin Moore, Andrew Phelps, Manoj Plakal, Daniel Sorin, Haris Volos, 
    Min Xu, and Luke Yen.
    --------------------------------------------------------------------

    If your use of this software contributes to a published paper, we
    request that you (1) cite our summary paper that appears on our
    website (http://www.cs.wisc.edu/gems/) and (2) e-mail a citation
    for your published paper to gems@cs.wisc.edu.

    If you redistribute derivatives of this software, we request that
    you notify us and either (1) ask people to register with us at our
    website (http://www.cs.wisc.edu/gems/) or (2) collect registration
    information and periodically send it to us.

    --------------------------------------------------------------------

    Multifacet GEMS is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    Multifacet GEMS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Multifacet GEMS; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307, USA

    The GNU General Public License is contained in the file LICENSE.

### END HEADER ###
*/

/*
 * $Id$
 */

%{

#include <assert.h>
#include "ASTs.h"
#include "parser.h"
#include <string>

extern "C" int yylex();
extern "C" void yyerror();
extern "C" int yywrap()
{
    return 1;
}

%}
%x CMNT
%x IMBEDED
%%

[\t ]+   /* Ignore whitespace */ 
[\n]       { g_line_number++; }  
"//".*[\n] { g_line_number++; }  /* C++ style comments */

"/*" BEGIN CMNT;
<CMNT>. ;
<CMNT>\n { g_line_number++; }  
<CMNT>"*/" { BEGIN INITIAL; }

true       { yylval.str_ptr = new string(yytext); return LIT_BOOL; }
false      { yylval.str_ptr = new string(yytext); return LIT_BOOL; }
global     { return GLOBAL_DECL; }
machine    { return MACHINE_DECL; }
in_port    { return IN_PORT_DECL; }
out_port   { return OUT_PORT_DECL; }
action     { return ACTION_DECL; }
transition { return TRANSITION_DECL; }
structure  { return STRUCT_DECL; }
external_type { return EXTERN_TYPE_DECL; }
enumeration { return ENUM_DECL; }
peek       { return PEEK; }
enqueue    { return ENQUEUE; }
copy_head  { return COPY_HEAD; }
check_allocate  { return CHECK_ALLOCATE; }
check_stop_slots  { return CHECK_STOP_SLOTS; }
if         { return IF; }
else       { return ELSE; }
return     { return RETURN; }
THIS       { return THIS; }
CHIP       { return CHIP; }
void       { yylval.str_ptr = new string(yytext); return VOID; }

==        { yylval.str_ptr = new string(yytext); return EQ; }
!=        { yylval.str_ptr = new string(yytext); return NE; }
[<]       { yylval.str_ptr = new string(yytext); return '<'; }
[>]       { yylval.str_ptr = new string(yytext); return '>'; }
[<][<]    { yylval.str_ptr = new string(yytext); return LEFTSHIFT; }
[>][>]    { yylval.str_ptr = new string(yytext); return RIGHTSHIFT; }
[<][=]    { yylval.str_ptr = new string(yytext); return LE; }
[>][=]    { yylval.str_ptr = new string(yytext); return GE; }
[!]       { yylval.str_ptr = new string(yytext); return NOT; }
[&][&]    { yylval.str_ptr = new string(yytext); return AND; }
[|][|]    { yylval.str_ptr = new string(yytext); return OR; }
[+]       { yylval.str_ptr = new string(yytext); return PLUS; }
[-]       { yylval.str_ptr = new string(yytext); return DASH; }
[*]       { yylval.str_ptr = new string(yytext); return STAR; }
[/]       { yylval.str_ptr = new string(yytext); return SLASH; }
::        { return DOUBLE_COLON; }
[:]       { return ':'; } 
[;]       { return SEMICOLON; }
[[]       { return '['; }
[]]       { return ']'; }
[{]       { return '{'; }
[}]       { return '}'; }
[(]       { return '('; }
[)]       { return ')'; }
[,]       { return ','; }
[=]       { return '='; }
:=        { return ASSIGN; }
[.]       { return DOT; }

[0-9]*[.][0-9]*     { yylval.str_ptr = new string(yytext); return FLOATNUMBER; }
[0-9]*     { yylval.str_ptr = new string(yytext); return NUMBER; }
[a-zA-Z_][a-zA-Z_0-9]{0,50}    { yylval.str_ptr = new string(yytext); return IDENT; }
\"[^"\n]*\"  { yytext[strlen(yytext)-1] = '\0';  yylval.str_ptr = new string(yytext+1); return STRING; }
\'[^'\n]*\'  { yytext[strlen(yytext)-1] = '\0';  yylval.str_ptr = new string(yytext+1); return STRING; }

.         { return OTHER; }  /* Need so that we handle all characters */

%% 

