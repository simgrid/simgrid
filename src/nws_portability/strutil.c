/* $Id$ */

#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "strutil.h"


#define EOS '\0'


void
strcase(char *string,
        CaseTypes toWhatCase) {

  char *c;
  int raiseIt;

  raiseIt = (toWhatCase == ALL_UPPER) || (toWhatCase == INITIAL_UPPER);

  for(c = string; *c != EOS; c++) {
    *c = raiseIt ? toupper((int)*c) : tolower((int)*c);
    raiseIt = (toWhatCase == ALL_UPPER) ||
              ((toWhatCase == INITIAL_UPPER) && !isalpha((int)*c));
  }

}


int
strnmatch(const char *string,
          const char *pattern,
          size_t len) {

  const char *lastChance;
  const char *nextWild;
  int tameLen;

  if(*pattern == '*') {
    do pattern++; while (*pattern == '*');
    if(*pattern == '\0')
      return 1; /* Trailing star matches everything. */
    nextWild = strchr(pattern, '*');
    tameLen = (nextWild == NULL) ? strlen(pattern) : nextWild - pattern;
    /*
    ** The wildcard we're processing can match any string of characters up to
    ** an occurrence of the following non-wild subpattern.  For each subpattern
    ** occurrence, see if the remaining string matches the remaining pattern.
    */
    lastChance = string + len - tameLen;
    for(; string <= lastChance; string++) {
      if((strncmp(string, pattern, tameLen) == 0) &&
         strnmatch(string + tameLen, pattern + tameLen, lastChance - string)) {
        return 1;
      }
    }
    return 0;
  }
  else {
    nextWild = strchr(pattern, '*');
    if(nextWild == NULL)
      /* No wildcards in pattern; check for exact match. */
      return (strlen(pattern) == len) && (strncmp(string, pattern, len) == 0);
    else {
      /* Check for leading exact match followed by a wildcard match. */
      tameLen = nextWild - pattern;
      return (tameLen <= len) &&
             (strncmp(string, pattern, tameLen) == 0) &&
             strnmatch(string + tameLen, nextWild, len - tameLen);
    }
  }

}


#ifndef HAVE_STRNLEN
size_t
strnlen(	const char *s,
		size_t maxlen) {
	size_t i;

	if (maxlen <= 0 || s == NULL) {
		return 0;
	}

	for (i=0; i<maxlen; i++) {
		if (s[i] == '\0') {
			/* done */
			break;
		}
	}
	return i;
}
#endif

int
strntok(char *dest,
        const char *source,
        int len,
        const char *delim,
        const char **end) {

	char *last = dest + len - 1;

	/* Skip leading delimiters. */
	while( (*source != EOS) && (strchr(delim, *source) != NULL) ) {
		source++;
	}

	if(*source == EOS) {
		return 0;
	}

	while( (dest < last) && (*source != EOS) &&
			(strchr(delim, *source) == NULL) ) {
		*dest++ = *source++;
	}

	if(end != NULL) {
		*end = (const char *)source;
	}
	*dest = EOS;

	return 1;
}


int
vstrncpy(char *dest,
         size_t len,
         int count,
         ...) {

  va_list paramList;
  int i;
  const char *source;
  char *end = dest + len - 1;
  char *start = dest;

  va_start(paramList, count);
  for (i = 0; i < count; i++) {
    for (source = va_arg(paramList, const char*);
         (dest < end) && (*source != EOS); dest++, source++)
      *dest = *source;
  }
  *dest = EOS;
  va_end(paramList);
  return dest - start;

}
