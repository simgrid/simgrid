/* $Id$ */


#ifndef STRUTIL_H
#define STRUTIL_H


/*
** This module defines some useful routines for maniputing strings.
*/

#include <string.h>     /* strlen() strncpy() */
#include <sys/types.h>  /* size_t */


#ifdef __cplusplus
extern "C" {
#endif


/*
** Replaces the characters in #string# with their lower- or upper-case
** equivalents.  #toWhatCase# specifies whether the result to be ENTIRELY
** UPPER, entirely lower, iNITIAL lOWER, or Initial Upper.
*/
typedef enum {ALL_LOWER, ALL_UPPER, INITIAL_LOWER, INITIAL_UPPER} CaseTypes;
void
strcase(char *string,
        CaseTypes toWhatCase);


/*
** Returns 1 or 0 depending on whether or not the first #len# characters of
** #string# matches #pattern#, which may contain wildcard characters (*).
*/
int
strnmatch(const char *string,
          const char *pattern,
          size_t len);
#define strmatch(string,pattern) strnmatch(string, pattern, strlen(string))


/*
** Copies a "token" -- a series of characters deliminated by one of the chars
** in #delim# -- from #source# to the #len#-long string #dest#.  Terminates
** #dest# with a null character.  Returns the position within #source# of the
** character after the token in #end#.  Skips any leading delimiters in
** #source#.  Returns 0 if the end of source is reached without finding any
** non-delimiter characters, 1 otherwise.
*/
int
strntok(char *dest,
        const char *source,
        int len,
        const char *delim,
        const char **end);
#define GETTOK(dest,source,delim,end) strntok(dest,source,sizeof(dest),delim,end)
#define GETWORD(dest,source,end) GETTOK(dest,source," \t\n",end)


/*
 * Define strnlen in case there is no such a function in the library
 * (sometimes there is not the definition in the include so we define it
 * anyway).
 */
size_t
strnlen(const char *s, size_t maxlen);

/*
** Calls strncpy() passing #dest#, #src#, and #len#, then places a terminating
** character in the last (len - 1) byte of #dest#.
*/
#define zstrncpy(dest,src,len) \
  do {strncpy(dest, src, len); dest[len - 1] = '\0'; if (1) break;} while (0)
#define SAFESTRCPY(dest,src) zstrncpy(dest, src, sizeof(dest))


/*
** Catenates the #count#-long set of source strings into #dest#.  Copies at
** most #len# characters, including the null terminator.
*/
int
vstrncpy(char *dest,
         size_t len,
         int count,
         ...);


#ifdef __cplusplus
}
#endif


#endif
