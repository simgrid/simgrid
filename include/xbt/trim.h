/* xbt/trim.h -- Declarations of the functions ltrim(), rtrim() and trim()	*/

/* Copyright (c) 2007 Cherier Malek. All rights reserved.					*/

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef XBT_TRIM_H
#define XBT_TRIM_H

#include "xbt/misc.h"

#ifdef __cplusplus
extern "C" {
#endif

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
XBT_PUBLIC(rtrim)(char* s, const char* char_list);

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
XBT_PUBLIC(ltrim)( char* s, const char* char_list);


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
XBT_PUBLIC(trim)(char* s, const char* char_list);


#ifdef __cplusplus
}
#endif

#endif /* !XBT_TRIM_H */ 