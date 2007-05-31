
/* xbt_str.c - various helping functions to deal with strings               */

/* Copyright (C) 2005-2007 Malek Cherier, Martin Quinson.                   */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

/* Returns the string without leading whitespaces (xbt_str_ltrim), 
 * trailing whitespaces (xbt_str_rtrim),
 * or both leading and trailing whitespaces (xbt_str_trim). 
 * (in-place modification of the string)
 */
  
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/str.h" /* headers of these functions */
#include "portable.h"
#include "xbt/matrix.h" /* for the diff */

static void free_string(void *d){
  free(*(void**)d);
}

/**  @brief Strip whitespace (or other characters) from the end of a string.
 *
 * Strips the whitespaces from the end of s. 
 * By default (when char_list=NULL), these characters get stripped:
 * 	
 *	- " "		(ASCII 32	(0x20))	space. 
 *	- "\t"		(ASCII 9	(0x09))	tab. 
 *	- "\n"		(ASCII 10	(0x0A))	line feed. 
 *	- "\r"		(ASCII 13	(0x0D))	carriage return. 
 *	- "\0"		(ASCII 0	(0x00))	NULL. 
 *	- "\x0B"	(ASCII 11	(0x0B))	vertical tab. 
 *
 * @param s The string to strip. Modified in place.
 * @param char_list A string which contains the characters you want to strip.
 *
 */
void
xbt_str_rtrim(char* s, const char* char_list)
{
	char* cur = s;
	const char* __char_list = " \t\n\r\x0B";
	char white_char[256] = {1,0};
	
	if(!s)
		return;

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

	while((cur >= s) && white_char[(unsigned char)*cur])
		--cur;

	*++cur = '\0';
}

/**  @brief Strip whitespace (or other characters) from the beginning of a string.
 *
 * Strips the whitespaces from the begining of s.
 * By default (when char_list=NULL), these characters get stripped:
 * 	
 *	- " "		(ASCII 32	(0x20))	space. 
 *	- "\t"		(ASCII 9	(0x09))	tab. 
 *	- "\n"		(ASCII 10	(0x0A))	line feed. 
 *	- "\r"		(ASCII 13	(0x0D))	carriage return. 
 *	- "\0"		(ASCII 0	(0x00))	NULL. 
 *	- "\x0B"	(ASCII 11	(0x0B))	vertical tab. 
 *
 * @param s The string to strip. Modified in place.
 * @param char_list A string which contains the characters you want to strip.
 *
 */
void
xbt_str_ltrim( char* s, const char* char_list)
{
	char* cur = s;
	const char* __char_list = " \t\n\r\x0B";
	char white_char[256] = {1,0};
	
	if(!s)
		return;

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

	memmove(s,cur, strlen(cur));
}

/**  @brief Strip whitespace (or other characters) from the end and the begining of a string.
 *
 * Strips the whitespaces from both the beginning and the end of s.
 * By default (when char_list=NULL), these characters get stripped:
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
 */
void
xbt_str_trim(char* s, const char* char_list ){
	
	if(!s)
		return;
		
    xbt_str_rtrim(s,char_list);
    xbt_str_ltrim(s,char_list);
}

/**  @brief Replace double whitespaces (but no other characters) from the string.
 *
 * The function modifies the string so that each time that several spaces appear,
 * they are replaced by a single space. It will only do so for spaces (ASCII 32, 0x20). 
 *
 * @param s The string to strip. Modified in place.
 *
 */
void
xbt_str_strip_spaces(char *s) {
  char *p = s;
  int   e = 0;

  if (!s)
    return;

  while (1) {
    if (!*p)
      goto end;
    
    if (*p != ' ')
      break;
    
    p++;
  }
  
  e = 1;
  
  do {
    if (e)
      *s++ = *p;
    
    if (!*++p)
      goto end;
    
    if (e ^ (*p!=' '))
      if ((e = !e))
	*s++ = ' ';
  } while (1);

 end:
  *s = '\0';
}

/** @brief Splits a string into a dynar of strings 
 * 
 * @param s: the string to split
 * @param sep: a string of all chars to consider as separator.
 *
 * By default (with sep=NULL), these characters are used as separator: 
 * 	
 *	- " "		(ASCII 32	(0x20))	space. 
 *	- "\t"		(ASCII 9	(0x09))	tab. 
 *	- "\n"		(ASCII 10	(0x0A))	line feed. 
 *	- "\r"		(ASCII 13	(0x0D))	carriage return. 
 *	- "\0"		(ASCII 0	(0x00))	NULL. 
 *	- "\x0B"	(ASCII 11	(0x0B))	vertical tab. 
 */

xbt_dynar_t xbt_str_split(const char *s, const char *sep) {
  xbt_dynar_t res = xbt_dynar_new(sizeof(char*), free_string);
  const char *p, *q;
  int done;
  const char* sep_dflt = " \t\n\r\x0B";
  char is_sep[256] = {1,0};

  /* check what are the separators */
  memset(is_sep,0,sizeof(is_sep));
  if (!sep) {
    while (*sep_dflt)
      is_sep[(unsigned char) *sep_dflt++] = 1;
  } else {
    while (*sep)
      is_sep[(unsigned char) *sep++] = 1;
  }
  is_sep[0] = 1; /* End of string is also separator */
   
  /* Do the job */
  p=q=s; 
  done=0;

  if (s[0] == '\0')
    return res;

  while (!done) {
    char *topush;
    while (!is_sep[(unsigned char)*q]) {
      q++;
    }
    if (*q == '\0')
      done = 1;

    topush=xbt_malloc(q-p+1);
    memcpy(topush,p,q-p);
    topush[q - p] = '\0';
    xbt_dynar_push(res,&topush);
    p = ++q;
  }

  return res;
}

/** @brief Join a set of strings as a single string */

char *xbt_str_join(xbt_dynar_t dyn, const char*sep) {
  int len=2;
  int cpt;
  char *cursor;
  char *res,*p;
  /* compute the length */
  xbt_dynar_foreach(dyn,cpt,cursor) {
    len+=strlen(cursor);
  }
  len+=strlen(sep)*(xbt_dynar_length(dyn));
  /* Do the job */
  res = xbt_malloc(len);
  p=res;
  xbt_dynar_foreach(dyn,cpt,cursor) {
    p+=sprintf(p,"%s%s",cursor,sep);    
  }
  return res;
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

/*
 * Diff related functions 
 */
static xbt_matrix_t diff_build_LCS(xbt_dynar_t da, xbt_dynar_t db) {
  xbt_matrix_t C = xbt_matrix_new(xbt_dynar_length(da),xbt_dynar_length(db),
				  sizeof(int),NULL); 
  int i,j;
  /* Compute the LCS */
  /*
    C = array(0..m, 0..n)
    for i := 0..m
       C[i,0] = 0
    for j := 1..n
       C[0,j] = 0
    for i := 1..m
        for j := 1..n
            if X[i] = Y[j]
                C[i,j] := C[i-1,j-1] + 1
            else:
                C[i,j] := max(C[i,j-1], C[i-1,j])
    return C[m,n]
	  */
  for (i=0; i<xbt_dynar_length(da); i++) 
    *((int*) xbt_matrix_get_ptr(C,i,0) ) = 0;

  for (j=0; j<xbt_dynar_length(db); j++) 
    *((int*) xbt_matrix_get_ptr(C,0,j) ) = 0;

  for (i=1; i<xbt_dynar_length(da); i++) 
    for (j=1; j<xbt_dynar_length(db); j++) {

      if (!strcmp(xbt_dynar_get_as(da,i,char*), xbt_dynar_get_as(db,j,char*)))
	*((int*) xbt_matrix_get_ptr(C,i,j) ) = xbt_matrix_get_as(C,i-1,j-1,int) + 1;
      else
	*((int*) xbt_matrix_get_ptr(C,i,j) ) = max(xbt_matrix_get_as(C,i  ,j-1,int),
						   xbt_matrix_get_as(C,i-1,j,int));
    }
  return C;
}

static void diff_build_diff(xbt_dynar_t res,
			    xbt_matrix_t C,
			    xbt_dynar_t da, xbt_dynar_t db,
			    int i,int j) {
  char *topush;
  /* Construct the diff 
  function printDiff(C[0..m,0..n], X[1..m], Y[1..n], i, j)
    if i > 0 and j > 0 and X[i] = Y[j]
        printDiff(C, X, Y, i-1, j-1)
        print "  " + X[i]
    else
        if j > 0 and (i = 0 or C[i,j-1] >= C[i-1,j])
            printDiff(C, X, Y, i, j-1)
            print "+ " + Y[j]
        else if i > 0 and (j = 0 or C[i,j-1] < C[i-1,j])
            printDiff(C, X, Y, i-1, j)
            print "- " + X[i]
  */

  if (i>=0 && j >= 0 && !strcmp(xbt_dynar_get_as(da,i,char*),
				xbt_dynar_get_as(db,j,char*))) {
    diff_build_diff(res,C,da,db,i-1,j-1);
    topush = bprintf("  %s",xbt_dynar_get_as(da,i,char*));
    xbt_dynar_push(res, &topush);
  } else if (j>=0 && 
	     (i<=0 || xbt_matrix_get_as(C,i,j-1,int) >= xbt_matrix_get_as(C,i-1,j,int))) {
    diff_build_diff(res,C,da,db,i,j-1);
    topush = bprintf("+ %s",xbt_dynar_get_as(db,j,char*));
    xbt_dynar_push(res,&topush);
  } else if (i>=0 && 
	     (j<=0 || xbt_matrix_get_as(C,i,j-1,int) < xbt_matrix_get_as(C,i-1,j,int))) {
    diff_build_diff(res,C,da,db,i-1,j);
    topush = bprintf("- %s",xbt_dynar_get_as(da,i,char*));
    xbt_dynar_push(res,&topush);        
  } else if (i<=0 && j<=0) {
    return;
  } else {
    THROW2(arg_error,0,"Invalid values: i=%d, j=%d",i,j);
  }
   
}

/** @brief Compute the unified diff of two strings */
char *xbt_str_diff(char *a, char *b) {
  xbt_dynar_t da = xbt_str_split(a, "\n");
  xbt_dynar_t db = xbt_str_split(b, "\n");

  xbt_matrix_t C = diff_build_LCS(da,db);
  xbt_dynar_t diff = xbt_dynar_new(sizeof(char*),free_string);
  char *res=NULL;
  
  diff_build_diff(diff, C, da,db, xbt_dynar_length(da)-1, xbt_dynar_length(db)-1);
  /* Clean empty lines at the end */
  while (xbt_dynar_length(diff) > 0) {
     char *str;
     xbt_dynar_pop(diff,&str);
     if (str[0]=='\0' || !strcmp(str,"  ")) {
	free(str);
     } else {
	xbt_dynar_push(diff,&str);
	break;
     }     
  }   
  res = xbt_str_join(diff, "\n");

  xbt_dynar_free(&da);
  xbt_dynar_free(&db);
  xbt_dynar_free(&diff);
  xbt_matrix_free(C);
  
  return res;
}
