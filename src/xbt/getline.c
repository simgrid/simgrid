/* $Id$ */

/* getline -- reimplementation of the GNU's getline() for other platforms   */

/* Copyright (C) 2007 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @brief  Reads an entire line.
 * 
 * Reads a line from the file specified by the file pointer passed
 * as parameter. This function is intead to replace the non-portable
 * GNU getline() function.
 * 
 * @param buf address to a string buffer. If null, it will be allocated to receive the read data. If not and if too short, it will be reallocated. Must be freed by caller.
 * @param n size of the buffer. Updated accordingly by the function.
 * @param stream File pointer to the file to read from.
 * @return The amount of chars read (or -1 on failure, ie on end of file condition)
 */
#include "xbt/misc.h"
#include "xbt/sysdep.h" /* headers of this function */
#include "portable.h"

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
