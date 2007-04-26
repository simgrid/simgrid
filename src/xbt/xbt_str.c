
/* xbt_str.c - various helping functions to deal with strings               */

/* Copyright (C) 2005-2007 Malek Cherier, Martin Quinson.                   */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

/* Returns a copy of a string without leading whitespaces (ltrim), trailing whitespaces (rtrim), 
 * or both leading and trailing whitespaces (trim).
 */
  
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/str.h" /* headers of these functions */
#include "portable.h"

/**  @brief Strip whitespace (or other characters) from the end of a string.
 *
 * The function rtrim() returns a string with whitespace stripped from the end of s. 
 * By default (without the second parameter char_list), rtrim() will strip these characters :
 * 	
 *	- " "		(ASCII 32	(0x20))	space. 
 *	- "\t"		(ASCII 9	(0x09))	tab. 
 *	- "\n"		(ASCII 10	(0x0A))	line feed. 
 *	- "\r"		(ASCII 13	(0x0D))	carriage return. 
 *	- "\0"		(ASCII 0	(0x00))	NULL. 
 *	- "\x0B"	(ASCII 11	(0x0B))	vertical tab. 
 *
 * @param s The string to strip.
 * @param char_list A string which contains the characters you want to strip.
 *
 * @return If the specified is NULL the function returns NULL. Otherwise the
 * function returns the string with whitespace stripped from the end.
 */
char*
rtrim(char* s, const char* char_list)
{
	char* cur = s;
	const char* __char_list = " \t\n\r\x0B";
	char white_char[256] = {1,0};
	
	if(!s)
		return NULL;

	if(!char_list){
		while(*__char_list) {
			white_char[(unsigned char)*__char_list++] = 1;
		}
	}else{
		while(*char_list) {
			white_char[(unsigned char)*char_list++] = 1;
		}
	}

	while(*cur)
		++cur;

	while(white_char[(unsigned char)*cur] && (cur >= s))
		--cur;

	*++cur = '\0';
	return s;
}

/**  @brief Strip whitespace (or other characters) from the beginning of a string.
 *
 * The function ltrim() returns a string with whitespace stripped from the beginning of s. 
 * By default (without the second parameter char_list), ltrim() will strip these characters :
 * 	
 *	- " "		(ASCII 32	(0x20))	space. 
 *	- "\t"		(ASCII 9	(0x09))	tab. 
 *	- "\n"		(ASCII 10	(0x0A))	line feed. 
 *	- "\r"		(ASCII 13	(0x0D))	carriage return. 
 *	- "\0"		(ASCII 0	(0x00))	NULL. 
 *	- "\x0B"	(ASCII 11	(0x0B))	vertical tab. 
 *
 * @param s The string to strip.
 * @param char_list A string which contains the characters you want to strip.
 *
 * @return If the specified is NULL the function returns NULL. Otherwise the
 * function returns the string with whitespace stripped from the beginning.
 */
char*
ltrim( char* s, const char* char_list)
{
	char* cur = s;
	const char* __char_list = " \t\n\r\x0B";
	char white_char[256] = {1,0};
	
	if(!s)
		return NULL;

	if(!char_list){
		while(*__char_list) {
			white_char[(unsigned char)*__char_list++] = 1;
		}
	}else{
		while(*char_list) {
			white_char[(unsigned char)*char_list++] = 1;
		}
	}

	while(*cur && white_char[(unsigned char)*cur])
		++cur;

	return strcpy(s,cur);
}

/**  @brief Strip whitespace (or other characters) from the end and the begining of a string.
 *
 * The function trim() returns a string with whitespace stripped from the end and the begining of s. 
 * By default (without the second parameter char_list), trim() will strip these characters :
 * 	
 *	- " "		(ASCII 32	(0x20))	space. 
 *	- "\t"		(ASCII 9	(0x09))	tab. 
 *	- "\n"		(ASCII 10	(0x0A))	line feed. 
 *	- "\r"		(ASCII 13	(0x0D))	carriage return. 
 *	- "\0"		(ASCII 0	(0x00))	NULL. 
 *	- "\x0B"	(ASCII 11	(0x0B))	vertical tab. 
 *
 * @param s The string to strip.
 * @param char_list A string which contains the characters you want to strip.
 *
 * @return If the specified is NULL the function returns NULL. Otherwise the
 * function returns the string with whitespace stripped from the end and the begining.
 */
char* 
trim(char* s, const char* char_list ){
	
	if(!s)
		return NULL;
		
    return ltrim(rtrim(s,char_list),char_list);
}
   
#ifndef HAVE_GETLINE
long getline(char **buf, size_t *n, FILE *stream) {
   
   int i, ch;
   
   if (!*buf) {
     *buf = xbt_malloc(512);
     *n = 512;
   }
   
   if (feof(stream))
     return (ssize_t)-1;
   
   for (i=0; (ch = fgetc(stream)) != EOF; i++)  {
	
      if (i >= (*n) + 1)
	*buf = xbt_realloc(*buf, *n += 512);
	
      (*buf)[i] = ch;
	
      if ((*buf)[i] == '\n')  {
	 i++;
	 (*buf)[i] = '\0';
	 break;
      }      
   }
      
   if (i == *n) 
     *buf = xbt_realloc(*buf, *n += 1);
   
   (*buf)[i] = '\0';

   return (ssize_t)i;
}

#endif /* HAVE_GETLINE */
