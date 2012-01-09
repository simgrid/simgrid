
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NEVER = 258,
     IF = 259,
     FI = 260,
     IMPLIES = 261,
     GOTO = 262,
     AND = 263,
     OR = 264,
     NOT = 265,
     LEFT_PAR = 266,
     RIGHT_PAR = 267,
     CASE = 268,
     COLON = 269,
     SEMI_COLON = 270,
     CASE_TRUE = 271,
     LEFT_BRACE = 272,
     RIGHT_BRACE = 273,
     LITT_ENT = 274,
     LITT_CHAINE = 275,
     LITT_REEL = 276,
     ID = 277
   };
#endif
/* Tokens.  */
#define NEVER 258
#define IF 259
#define FI 260
#define IMPLIES 261
#define GOTO 262
#define AND 263
#define OR 264
#define NOT 265
#define LEFT_PAR 266
#define RIGHT_PAR 267
#define CASE 268
#define COLON 269
#define SEMI_COLON 270
#define CASE_TRUE 271
#define LEFT_BRACE 272
#define RIGHT_BRACE 273
#define LITT_ENT 274
#define LITT_CHAINE 275
#define LITT_REEL 276
#define ID 277




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 11 "parserPromela.yacc"

  double real;
  int integer;
  char* string;
  xbt_exp_label_t label;



/* Line 1676 of yacc.c  */
#line 105 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


