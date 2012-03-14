/* xbt_str.c - various helping functions to deal with strings               */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/str.h"            /* headers of these functions */
#include "xbt/strbuff.h"
#include "xbt/matrix.h"         /* for the diff */

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
void xbt_str_rtrim(char *s, const char *char_list)
{
  char *cur = s;
  const char *__char_list = " \t\n\r\x0B";
  char white_char[256] = { 1, 0 };

  if (!s)
    return;

  if (!char_list) {
    while (*__char_list) {
      white_char[(unsigned char) *__char_list++] = 1;
    }
  } else {
    while (*char_list) {
      white_char[(unsigned char) *char_list++] = 1;
    }
  }

  while (*cur)
    ++cur;

  while ((cur >= s) && white_char[(unsigned char) *cur])
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
void xbt_str_ltrim(char *s, const char *char_list)
{
  char *cur = s;
  const char *__char_list = " \t\n\r\x0B";
  char white_char[256] = { 1, 0 };

  if (!s)
    return;

  if (!char_list) {
    while (*__char_list) {
      white_char[(unsigned char) *__char_list++] = 1;
    }
  } else {
    while (*char_list) {
      white_char[(unsigned char) *char_list++] = 1;
    }
  }

  while (*cur && white_char[(unsigned char) *cur])
    ++cur;

  memmove(s, cur, strlen(cur) + 1);
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
void xbt_str_trim(char *s, const char *char_list)
{

  if (!s)
    return;

  xbt_str_rtrim(s, char_list);
  xbt_str_ltrim(s, char_list);
}

/**  @brief Replace double whitespaces (but no other characters) from the string.
 *
 * The function modifies the string so that each time that several spaces appear,
 * they are replaced by a single space. It will only do so for spaces (ASCII 32, 0x20).
 *
 * @param s The string to strip. Modified in place.
 *
 */
void xbt_str_strip_spaces(char *s)
{
  char *p = s;
  int e = 0;

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

    if (e ^ (*p != ' '))
      if ((e = !e))
        *s++ = ' ';
  } while (1);

end:
  *s = '\0';
}

/** @brief Substitutes a char for another in a string
 *
 * @param str the string to modify
 * @param from char to search
 * @param to char to put instead
 * @param occurence number of changes to do (=0 means all)
 */
void xbt_str_subst(char *str, char from, char to, int occurence)
{
  char *p = str;
  while (*p != '\0') {
    if (*p == from) {
      *p = to;
      if (occurence == 1)
        return;
      occurence--;
    }
    p++;
  }
}

/** @brief Replaces a set of variables by their values
 *
 * @param str The input of the replacement process
 * @param patterns The changes to apply
 * @return The string modified
 *
 * Both '$toto' and '${toto}' are valid (and the two writing are equivalent).
 *
 * If the variable name contains spaces, use the brace version (ie, ${toto tutu})
 *
 * You can provide a default value to use if the variable is not set in the dict by using
 * '${var:=default}' or '${var:-default}'. These two forms are equivalent, even if they
 * shouldn't to respect the shell standard (:= form should set the value in the dict,
 * but does not) (BUG).
 */

char *xbt_str_varsubst(const char *str, xbt_dict_t patterns)
{
  xbt_strbuff_t buff = xbt_strbuff_new_from(str);
  char *res;
  xbt_strbuff_varsubst(buff, patterns);
  res = buff->data;
  xbt_strbuff_free_container(buff);
  return res;
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

xbt_dynar_t xbt_str_split(const char *s, const char *sep)
{
  xbt_dynar_t res = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  const char *p, *q;
  int done;
  const char *sep_dflt = " \t\n\r\x0B";
  char is_sep[256] = { 1, 0 };

  /* check what are the separators */
  memset(is_sep, 0, sizeof(is_sep));
  if (!sep) {
    while (*sep_dflt)
      is_sep[(unsigned char) *sep_dflt++] = 1;
  } else {
    while (*sep)
      is_sep[(unsigned char) *sep++] = 1;
  }
  is_sep[0] = 1;                /* End of string is also separator */

  /* Do the job */
  p = q = s;
  done = 0;

  if (s[0] == '\0')
    return res;

  while (!done) {
    char *topush;
    while (!is_sep[(unsigned char) *q]) {
      q++;
    }
    if (*q == '\0')
      done = 1;

    topush = xbt_malloc(q - p + 1);
    memcpy(topush, p, q - p);
    topush[q - p] = '\0';
    xbt_dynar_push(res, &topush);
    p = ++q;
  }

  return res;
}

/**
 * \brief This functions splits a string after using another string as separator
 * For example A!!B!!C splitted after !! will return the dynar {A,B,C}
 * \return An array of dynars containing the string tokens
 */
xbt_dynar_t xbt_str_split_str(const char *s, const char *sep)
{
  xbt_dynar_t res = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  int done;
  const char *p, *q;

  p = q = s;
  done = 0;

  if (s[0] == '\0')
    return res;
  if (sep[0] == '\0') {
    s = xbt_strdup(s);
    xbt_dynar_push(res, &s);
    return res;
  }

  while (!done) {
    char *to_push;
    int v = 0;
    //get the start of the first occurence of the substring
    q = strstr(p, sep);
    //if substring was not found add the entire string
    if (NULL == q) {
      v = strlen(p);
      to_push = malloc(v + 1);
      memcpy(to_push, p, v);
      to_push[v] = '\0';
      xbt_dynar_push(res, &to_push);
      done = 1;
    } else {
      //get the appearance
      to_push = malloc(q - p + 1);
      memcpy(to_push, p, q - p);
      //add string terminator
      to_push[q - p] = '\0';
      xbt_dynar_push(res, &to_push);
      p = q + strlen(sep);
    }
  }
  return res;
}

/** @brief Just like @ref xbt_str_split_quoted (Splits a string into a dynar of strings), but without memory allocation
 *
 * The string passed as argument must be writable (not const)
 * The elements of the dynar are just parts of the string passed as argument.
 *
 * To free the structure constructed by this function, free the first element and free the dynar:
 *
 * free(xbt_dynar_get_ptr(dynar,0));
 * xbt_dynar_free(&dynar);
 */
xbt_dynar_t xbt_str_split_quoted_in_place(char *s) {
  xbt_dynar_t res = xbt_dynar_new(sizeof(char *), NULL);
  char *beg, *end;              /* pointers around the parsed chunk */
  int in_simple_quote = 0, in_double_quote = 0;
  int done = 0;
  int ctn = 0;                  /* Got something in this block */

  if (s[0] == '\0')
    return res;

  beg = s;

  /* do not trim leading spaces: caller responsability to clean his cruft */
  end = beg;

  while (!done) {


    switch (*end) {
    case '\\':
      ctn = 1;
      /* Protected char; move it closer */
      memmove(end, end + 1, strlen(end));
      if (*end == '\0')
        THROWF(arg_error, 0, "String ends with \\");
      end++;                    /* Pass the protected char */
      break;

    case '\'':
      ctn = 1;
      if (!in_double_quote) {
        in_simple_quote = !in_simple_quote;
        memmove(end, end + 1, strlen(end));
      } else {
        /* simple quote protected by double ones */
        end++;
      }
      break;
    case '"':
      ctn = 1;
      if (!in_simple_quote) {
        in_double_quote = !in_double_quote;
        memmove(end, end + 1, strlen(end));
      } else {
        /* double quote protected by simple ones */
        end++;
      }
      break;

    case ' ':
    case '\t':
    case '\n':
    case '\0':
      if (*end == '\0' && (in_simple_quote || in_double_quote)) {
        THROWF(arg_error, 0,
               "End of string found while searching for %c in %s",
               (in_simple_quote ? '\'' : '"'), s);
      }
      if (in_simple_quote || in_double_quote) {
        end++;
      } else {
        if (*end == '\0')
          done = 1;

        *end = '\0';
        if (ctn) {
          /* Found a separator. Push the string if contains something */
          xbt_dynar_push(res, &beg);
        }
        ctn = 0;

        if (done)
          break;

        beg = ++end;
        /* trim within the string, manually to speed things up */
        while (*beg == ' ')
          beg++;
        end = beg;
      }
      break;

    default:
      ctn = 1;
      end++;
    }
  }
  return res;
}

/** @brief Splits a string into a dynar of strings, taking quotes into account
 *
 * It basically does the same argument separation than the shell, where white
 * spaces can be escaped and where arguments are never split within a
 * quote group.
 * Several subsequent spaces are ignored (unless within quotes, of course).
 * You may want to trim the input string, if you want to avoid empty entries
 *
 */

xbt_dynar_t xbt_str_split_quoted(const char *s)
{
  xbt_dynar_t res = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  xbt_dynar_t parsed;
  char *str_to_free;            /* we have to copy the string before, to handle backslashes */
  unsigned int cursor;
  char *p;

  if (s[0] == '\0')
    return res;
  str_to_free = xbt_strdup(s);

  parsed = xbt_str_split_quoted_in_place(str_to_free);
  xbt_dynar_foreach(parsed,cursor,p) {
    char *q=xbt_strdup(p);
    xbt_dynar_push(res,&q);
  }
  free(str_to_free);
  xbt_dynar_shrink(res, 0);
  xbt_dynar_free(&parsed);
  return res;
}

/** @brief Join a set of strings as a single string */
char *xbt_str_join(xbt_dynar_t dyn, const char *sep)
{
  int len = 1, dyn_len = xbt_dynar_length(dyn);
  unsigned int cpt;
  char *cursor;
  char *res, *p;

  if (!dyn_len)
    return xbt_strdup("");

  /* compute the length */
  xbt_dynar_foreach(dyn, cpt, cursor) {
    len += strlen(cursor);
  }
  len += strlen(sep) * dyn_len;
  /* Do the job */
  res = xbt_malloc(len);
  p = res;
  xbt_dynar_foreach(dyn, cpt, cursor) {
    if ((int) cpt < dyn_len - 1)
      p += sprintf(p, "%s%s", cursor, sep);
    else
      p += sprintf(p, "%s", cursor);
  }
  return res;
}
/** @brief Join a set of strings as a single string
 *
 * The parameter must be a NULL-terminated array of chars,
 * just like xbt_dynar_to_array() produces
 */
char *xbt_str_join_array(const char *const *strs, const char *sep)
{
  char *res,*q;
  int amount_strings=0;
  int len=0;
  int i;

  if ((!strs) || (!strs[0]))
    return xbt_strdup("");

  /* compute the length before malloc */
  for (i=0;strs[i];i++) {
    len += strlen(strs[i]);
    amount_strings++;
  }
  len += strlen(sep) * amount_strings;

  /* Do the job */
  q = res = xbt_malloc(len);
  for (i=0;strs[i];i++) {
    if (i!=0) { // not first loop
      q += sprintf(q, "%s%s", sep, strs[i]);
    } else {
      q += sprintf(q,"%s",strs[i]);
    }
  }
  return res;
}

#if defined(SIMGRID_NEED_GETLINE) || defined(DOXYGEN)
/** @brief Get a single line from the stream (reimplementation of the GNU getline)
 *
 * This is a redefinition of the GNU getline function, used on platforms where it does not exists.
 *
 * getline() reads an entire line from stream, storing the address of the buffer
 * containing the text into *buf.  The buffer is null-terminated and includes
 * the newline character, if one was found.
 *
 * If *buf is NULL, then getline() will allocate a buffer for storing the line,
 * which should be freed by the user program.  Alternatively, before calling getline(),
 * *buf can contain a pointer to a malloc()-allocated buffer *n bytes in size.  If the buffer
 * is not large enough to hold the line, getline() resizes it with realloc(), updating *buf and *n
 * as necessary.  In either case, on a successful call, *buf and *n will be updated to
 * reflect the buffer address and allocated size respectively.
 */
long getline(char **buf, size_t * n, FILE * stream)
{

  size_t i;
  int ch;

  if (!*buf) {
    *buf = xbt_malloc(512);
    *n = 512;
  }

  if (feof(stream))
    return (ssize_t) - 1;

  for (i = 0; (ch = fgetc(stream)) != EOF; i++) {

    if (i >= (*n) + 1)
      *buf = xbt_realloc(*buf, *n += 512);

    (*buf)[i] = ch;

    if ((*buf)[i] == '\n') {
      i++;
      (*buf)[i] = '\0';
      break;
    }
  }

  if (i == *n)
    *buf = xbt_realloc(*buf, *n += 1);

  (*buf)[i] = '\0';

  return (ssize_t) i;
}

#endif                          /* HAVE_GETLINE */

/*
 * Diff related functions
 */
static XBT_INLINE int diff_get(xbt_matrix_t C, int i, int j)
{
  return (i == -1 || j == -1) ? 0 : xbt_matrix_get_as(C, i, j, int);
}

static xbt_matrix_t diff_build_LCS(xbt_dynar_t da, xbt_dynar_t db,
                                   int len_a, int len_b)
{
  xbt_matrix_t C = xbt_matrix_new(len_a, len_b, sizeof(int), NULL);
  int i, j;

  /* Compute the LCS */
  /*
     function LCSLength(X[1..m], Y[1..n])
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
  for (i = 0; i < len_a; i++)
    for (j = 0; j < len_b; j++) {

      if (!strcmp(xbt_dynar_get_as(da, i, char *),
                  xbt_dynar_get_as(db, j, char *)))
        *((int *) xbt_matrix_get_ptr(C, i, j)) = diff_get(C, i - 1, j - 1) + 1;
      else
        *((int *) xbt_matrix_get_ptr(C, i, j)) = max(diff_get(C, i, j - 1),
                                                     diff_get(C, i - 1, j));
    }
  return C;
}

static void diff_build_diff(xbt_dynar_t res,
                            xbt_matrix_t C,
                            xbt_dynar_t da, xbt_dynar_t db, int i, int j)
{
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

  if (i >= 0 && j >= 0 && !strcmp(xbt_dynar_get_as(da, i, char *),
                                  xbt_dynar_get_as(db, j, char *))) {
    diff_build_diff(res, C, da, db, i - 1, j - 1);
    topush = bprintf("  %s", xbt_dynar_get_as(da, i, char *));
    xbt_dynar_push(res, &topush);
  } else if (j >= 0 &&
             (i == -1 || diff_get(C, i, j - 1) >= diff_get(C, i - 1, j))) {
    diff_build_diff(res, C, da, db, i, j - 1);
    topush = bprintf("+ %s", xbt_dynar_get_as(db, j, char *));
    xbt_dynar_push(res, &topush);
  } else if (i >= 0 &&
             (j == -1 || diff_get(C, i, j - 1) < diff_get(C, i - 1, j))) {
    diff_build_diff(res, C, da, db, i - 1, j);
    topush = bprintf("- %s", xbt_dynar_get_as(da, i, char *));
    xbt_dynar_push(res, &topush);
  }
}

/** @brief Compute the unified diff of two strings */
char *xbt_str_diff(const char *a, const char *b)
{
  xbt_dynar_t da = xbt_str_split(a, "\n");
  xbt_dynar_t db = xbt_str_split(b, "\n");
  xbt_matrix_t C;
  xbt_dynar_t diff;
  char *res;
  size_t len;
  unsigned length_da;

  diff = xbt_dynar_new(sizeof(char *), &xbt_free_ref);

  /* Clean empty lines at the end of da and db */
  len = strlen(a);
  if (len > 0 && a[len - 1] == '\n')
    xbt_dynar_pop(da, NULL);
  len = strlen(b);
  if (len > 0 && b[len - 1] == '\n')
    xbt_dynar_pop(db, NULL);

  /* Find the common suffix, do it before extracting the prefix,
   * as xbt_dynar_pop costs less than xbt_dynar_shift */
  length_da = xbt_dynar_length(da);
  while (length_da > 0 && xbt_dynar_length(db) > 0 &&
         !strcmp(xbt_dynar_get_as(da, length_da - 1, char *),
                 xbt_dynar_getlast_as(db, char *))) {
    xbt_dynar_pop(db, NULL);
    length_da--;
  }

  /* Extract the common prefix */
  while (length_da > 0 && xbt_dynar_length(db) > 0 &&
         !strcmp(xbt_dynar_getfirst_as(da, char *),
                 xbt_dynar_getfirst_as(db, char *))) {
    char *topush = bprintf("  %s", xbt_dynar_getfirst_as(da, char *));
    xbt_dynar_push(diff, &topush);
    xbt_dynar_shift(da, NULL);
    xbt_dynar_shift(db, NULL);
    length_da--;
  }

  /* Compute the diff for the remaining */
  C = diff_build_LCS(da, db, length_da, xbt_dynar_length(db));
  diff_build_diff(diff, C, da, db, length_da - 1, xbt_dynar_length(db) - 1);
  xbt_matrix_free(C);
  xbt_dynar_free(&db);

  /* Add the common suffix */
  for (; length_da < xbt_dynar_length(da); length_da++) {
    char *topush = bprintf("  %s", xbt_dynar_get_as(da, length_da, char *));
    xbt_dynar_push(diff, &topush);
  }
  xbt_dynar_free(&da);

  /* Build the final result */
  res = xbt_str_join(diff, "\n");
  xbt_dynar_free(&diff);

  return res;
}


/** @brief creates a new string containing what can be read on a fd
 *
 */
char *xbt_str_from_file(FILE * file)
{
  xbt_strbuff_t buff = xbt_strbuff_new();
  char *res;
  char bread[1024];
  memset(bread, 0, 1024);

  while (!feof(file)) {
    int got = fread(bread, 1, 1023, file);
    bread[got] = '\0';
    xbt_strbuff_append(buff, bread);
  }

  res = buff->data;
  xbt_strbuff_free_container(buff);
  return res;
}

#ifdef SIMGRID_TEST
#include "xbt/str.h"

#define mytest(name, input, expected) \
  xbt_test_add(name); \
  d=xbt_str_split_quoted(input); \
  s=xbt_str_join(d,"XXX"); \
  xbt_test_assert(!strcmp(s,expected),\
                   "Input (%s) leads to (%s) instead of (%s)", \
                   input,s,expected);\
                   free(s); \
                   xbt_dynar_free(&d);

XBT_TEST_SUITE("xbt_str", "String Handling");
XBT_TEST_UNIT("xbt_str_split_quoted", test_split_quoted, "test the function xbt_str_split_quoted")
{
  xbt_dynar_t d;
  char *s;

  mytest("Empty", "", "");
  mytest("Basic test", "toto tutu", "totoXXXtutu");
  mytest("Useless backslashes", "\\t\\o\\t\\o \\t\\u\\t\\u",
         "totoXXXtutu");
  mytest("Protected space", "toto\\ tutu", "toto tutu");
  mytest("Several spaces", "toto   tutu", "totoXXXtutu");
  mytest("LTriming", "  toto tatu", "totoXXXtatu");
  mytest("Triming", "  toto   tutu  ", "totoXXXtutu");
  mytest("Single quotes", "'toto tutu' tata", "toto tutuXXXtata");
  mytest("Double quotes", "\"toto tutu\" tata", "toto tutuXXXtata");
  mytest("Mixed quotes", "\"toto' 'tutu\" tata", "toto' 'tutuXXXtata");
  mytest("Backslashed quotes", "\\'toto tutu\\' tata",
         "'totoXXXtutu'XXXtata");
  mytest("Backslashed quotes + quotes", "'toto \\'tutu' tata",
         "toto 'tutuXXXtata");

}

#define mytest_str(name, input, separator, expected) \
  xbt_test_add(name); \
  d=xbt_str_split_str(input, separator); \
  s=xbt_str_join(d,"XXX"); \
  xbt_test_assert(!strcmp(s,expected),\
                   "Input (%s) leads to (%s) instead of (%s)", \
                   input,s,expected);\
                   free(s); \
                   xbt_dynar_free(&d);

XBT_TEST_UNIT("xbt_str_split_str", test_split_str, "test the function xbt_str_split_str")
{
  xbt_dynar_t d;
  char *s;

  mytest_str("Empty string and separator", "", "", "");
  mytest_str("Empty string", "", "##", "");
  mytest_str("Empty separator", "toto", "", "toto");
  mytest_str("String with no separator in it", "toto", "##", "toto");
  mytest_str("Basic test", "toto##tutu", "##", "totoXXXtutu");
}

/* Last args are format string and parameters for xbt_test_add */
#define mytest_diff(s1, s2, diff, ...)                                  \
  do {                                                                  \
    char *mytest_diff_res;                                              \
    xbt_test_add(__VA_ARGS__);                                          \
    mytest_diff_res = xbt_str_diff(s1, s2);                             \
    xbt_test_assert(!strcmp(mytest_diff_res, diff),                     \
                    "Wrong output:\n--- got:\n%s\n--- expected:\n%s\n---", \
                    mytest_diff_res, diff);                             \
    free(mytest_diff_res);                                              \
  } while (0)

XBT_TEST_UNIT("xbt_str_diff", test_diff, "test the function xbt_str_diff")
{
  unsigned i;

  /* Trivial cases */
  mytest_diff("a", "a", "  a", "1 word, no difference");
  mytest_diff("a", "A", "- a\n+ A", "1 word, different");
  mytest_diff("a\n", "a\n", "  a", "1 line, no difference");
  mytest_diff("a\n", "A\n", "- a\n+ A", "1 line, different");

  /* Empty strings */
  mytest_diff("", "", "", "empty strings");
  mytest_diff("", "a", "+ a", "1 word, added");
  mytest_diff("a", "", "- a", "1 word, removed");
  mytest_diff("", "a\n", "+ a", "1 line, added");
  mytest_diff("a\n", "", "- a", "1 line, removed");
  mytest_diff("", "a\nb\nc\n", "+ a\n+ b\n+ c", "4 lines, all added");
  mytest_diff("a\nb\nc\n", "", "- a\n- b\n- c", "4 lines, all removed");

  /* Empty lines */
  mytest_diff("\n", "\n", "  ", "empty lines");
  mytest_diff("", "\n", "+ ", "empty line, added");
  mytest_diff("\n", "", "- ", "empty line, removed");

  mytest_diff("a", "\na", "+ \n  a", "empty line added before word");
  mytest_diff("a", "a\n\n", "  a\n+ ", "empty line added after word");
  mytest_diff("\na", "a", "- \n  a", "empty line removed before word");
  mytest_diff("a\n\n", "a", "  a\n- ", "empty line removed after word");

  mytest_diff("a\n", "\na\n", "+ \n  a", "empty line added before line");
  mytest_diff("a\n", "a\n\n", "  a\n+ ", "empty line added after line");
  mytest_diff("\na\n", "a\n", "- \n  a", "empty line removed before line");
  mytest_diff("a\n\n", "a\n", "  a\n- ", "empty line removed after line");

  mytest_diff("a\nb\nc\nd\n", "\na\nb\nc\nd\n", "+ \n  a\n  b\n  c\n  d",
              "empty line added before 4 lines");
  mytest_diff("a\nb\nc\nd\n", "a\nb\nc\nd\n\n", "  a\n  b\n  c\n  d\n+ ",
              "empty line added after 4 lines");
  mytest_diff("\na\nb\nc\nd\n", "a\nb\nc\nd\n", "- \n  a\n  b\n  c\n  d",
              "empty line removed before 4 lines");
  mytest_diff("a\nb\nc\nd\n\n", "a\nb\nc\nd\n", "  a\n  b\n  c\n  d\n- ",
              "empty line removed after 4 lines");

  /* Missing newline at the end of one of the strings */
  mytest_diff("a\n", "a", "  a", "1 line, 1 word, no difference");
  mytest_diff("a", "a\n", "  a", "1 word, 1 line, no difference");
  mytest_diff("a\n", "A", "- a\n+ A", "1 line, 1 word, different");
  mytest_diff("a", "A\n", "- a\n+ A", "1 word, 1 line, different");

  mytest_diff("a\nb\nc\nd", "a\nb\nc\nd\n", "  a\n  b\n  c\n  d",
              "4 lines, no newline on first");
  mytest_diff("a\nb\nc\nd\n", "a\nb\nc\nd", "  a\n  b\n  c\n  d",
              "4 lines, no newline on second");

  /* Four lines, all combinations of differences */
  for (i = 0 ; i < (1U << 4) ; i++) {
    char d2[4 + 1];
    char s2[4 * 2 + 1];
    char res[4 * 8 + 1];
    char *pd = d2;
    char *ps = s2;
    char *pr = res;
    unsigned j = 0;
    while (j < 4) {
      unsigned k;
      for (/* j */ ; j < 4 && !(i & (1U << j)) ; j++) {
        *pd++ = "abcd"[j];
        ps += sprintf(ps, "%c\n", "abcd"[j]);
        pr += sprintf(pr, "  %c\n", "abcd"[j]);
      }
      for (k = j ; k < 4 && (i & (1U << k)) ; k++) {
        *pd++ = "ABCD"[k];
        ps += sprintf(ps, "%c\n", "ABCD"[k]);
        pr += sprintf(pr, "- %c\n", "abcd"[k]);
      }
      for (/* j */ ; j < k ; j++) {
        pr += sprintf(pr, "+ %c\n", "ABCD"[j]);
      }
    }
    *pd = '\0';
    *--pr = '\0';               /* strip last '\n' from expected result */
    mytest_diff("a\nb\nc\nd\n", s2, res,
                "compare (abcd) with changed (%s)", d2);
  }

  /* Subsets of four lines, do not test for empty subset */
  for (i = 1 ; i < (1U << 4) ; i++) {
    char d2[4 + 1];
    char s2[4 * 2 + 1];
    char res[4 * 8 + 1];
    char *pd = d2;
    char *ps = s2;
    char *pr = res;
    unsigned j = 0;
    while (j < 4) {
      for (/* j */ ; j < 4 && (i & (1U << j)) ; j++) {
        *pd++ = "abcd"[j];
        ps += sprintf(ps, "%c\n", "abcd"[j]);
        pr += sprintf(pr, "  %c\n", "abcd"[j]);
      }
      for (/* j */; j < 4 && !(i & (1U << j)) ; j++) {
        pr += sprintf(pr, "- %c\n", "abcd"[j]);
      }
    }
    *pd = '\0';
    *--pr = '\0';               /* strip last '\n' from expected result */
    mytest_diff("a\nb\nc\nd\n", s2, res,
                "compare (abcd) with subset (%s)", d2);

    for (pr = res ; *pr != '\0' ; pr++)
      if (*pr == '-')
        *pr = '+';
    mytest_diff(s2, "a\nb\nc\nd\n", res,
                "compare subset (%s) with (abcd)", d2);
  }
}

#endif                          /* SIMGRID_TEST */
