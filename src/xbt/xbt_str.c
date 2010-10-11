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
 * @param amount amount of changes to do (=0 means all)
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
 * @param str where to apply the change
 * @param patterns what to change
 * @return The string modified
 *
 * Check xbt_strbuff_varsubst() for more details, and remember that the string may be reallocated (moved) in the process.
 */

char *xbt_str_varsubst(char *str, xbt_dict_t patterns)
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

/** @brief Splits a string into a dynar of strings, taking quotes into account
 *
 * It basically does the same argument separation than the shell, where white
 * spaces can be escaped and where arguments are never splitted within a
 * quote group.
 * Several subsequent spaces are ignored (unless within quotes, of course).
 *
 */

xbt_dynar_t xbt_str_split_quoted(const char *s)
{
  xbt_dynar_t res = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  char *str_to_free;            /* we have to copy the string before, to handle backslashes */
  char *beg, *end;              /* pointers around the parsed chunk */
  int in_simple_quote = 0, in_double_quote = 0;
  int done = 0;
  int ctn = 0;                  /* Got something in this block */

  if (s[0] == '\0')
    return res;
  beg = str_to_free = xbt_strdup(s);

  /* trim leading spaces */
  xbt_str_ltrim(beg, " ");
  end = beg;

  while (!done) {


    switch (*end) {
    case '\\':
      ctn = 1;
      /* Protected char; move it closer */
      memmove(end, end + 1, strlen(end));
      if (*end == '\0')
        THROW0(arg_error, 0, "String ends with \\");
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
        THROW2(arg_error, 0,
               "End of string found while searching for %c in %s",
               (in_simple_quote ? '\'' : '"'), s);
      }
      if (in_simple_quote || in_double_quote) {
        end++;
      } else {
        if (ctn) {
          /* Found a separator. Push the string if contains something */
          char *topush = xbt_malloc(end - beg + 1);
          memcpy(topush, beg, end - beg);
          topush[end - beg] = '\0';
          xbt_dynar_push(res, &topush);
        }
        ctn = 0;

        if (*end == '\0') {
          done = 1;
          break;
        }

        beg = ++end;
        xbt_str_ltrim(beg, " ");
        end = beg;
      }
      break;

    default:
      ctn = 1;
      end++;
    }
  }
  free(str_to_free);
  xbt_dynar_shrink(res, 0);
  return res;
}

#ifdef SIMGRID_TEST
#include "xbt/str.h"

#define mytest(name, input, expected) \
  xbt_test_add0(name); \
  d=xbt_str_split_quoted(input); \
  s=xbt_str_join(d,"XXX"); \
  xbt_test_assert3(!strcmp(s,expected),\
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
  xbt_test_add0(name); \
  d=xbt_str_split_str(input, separator); \
  s=xbt_str_join(d,"XXX"); \
  xbt_test_assert3(!strcmp(s,expected),\
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
#endif                          /* SIMGRID_TEST */

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
static xbt_matrix_t diff_build_LCS(xbt_dynar_t da, xbt_dynar_t db)
{
  xbt_matrix_t C =
      xbt_matrix_new(xbt_dynar_length(da), xbt_dynar_length(db),
                     sizeof(int), NULL);
  unsigned long i, j;

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
  if (xbt_dynar_length(db) != 0)
    for (i = 0; i < xbt_dynar_length(da); i++)
      *((int *) xbt_matrix_get_ptr(C, i, 0)) = 0;

  if (xbt_dynar_length(da) != 0)
    for (j = 0; j < xbt_dynar_length(db); j++)
      *((int *) xbt_matrix_get_ptr(C, 0, j)) = 0;

  for (i = 1; i < xbt_dynar_length(da); i++)
    for (j = 1; j < xbt_dynar_length(db); j++) {

      if (!strcmp
          (xbt_dynar_get_as(da, i, char *),
           xbt_dynar_get_as(db, j, char *)))
        *((int *) xbt_matrix_get_ptr(C, i, j)) =
            xbt_matrix_get_as(C, i - 1, j - 1, int) + 1;
      else
        *((int *) xbt_matrix_get_ptr(C, i, j)) =
            max(xbt_matrix_get_as(C, i, j - 1, int),
                xbt_matrix_get_as(C, i - 1, j, int));
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
             (i <= 0 || j == 0
              || xbt_matrix_get_as(C, i, j - 1,
                                   int) >= xbt_matrix_get_as(C, i - 1, j,
                                                             int))) {
    diff_build_diff(res, C, da, db, i, j - 1);
    topush = bprintf("+ %s", xbt_dynar_get_as(db, j, char *));
    xbt_dynar_push(res, &topush);
  } else if (i >= 0 &&
             (j <= 0
              || xbt_matrix_get_as(C, i, j - 1, int) < xbt_matrix_get_as(C,
                                                                         i
                                                                         -
                                                                         1,
                                                                         j,
                                                                         int)))
  {
    diff_build_diff(res, C, da, db, i - 1, j);
    topush = bprintf("- %s", xbt_dynar_get_as(da, i, char *));
    xbt_dynar_push(res, &topush);
  } else if (i <= 0 && j <= 0) {
    return;
  } else {
    THROW2(arg_error, 0, "Invalid values: i=%d, j=%d", i, j);
  }

}

/** @brief Compute the unified diff of two strings */
char *xbt_str_diff(char *a, char *b)
{
  xbt_dynar_t da = xbt_str_split(a, "\n");
  xbt_dynar_t db = xbt_str_split(b, "\n");

  xbt_matrix_t C = diff_build_LCS(da, db);
  xbt_dynar_t diff = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  char *res = NULL;

  diff_build_diff(diff, C, da, db, xbt_dynar_length(da) - 1,
                  xbt_dynar_length(db) - 1);
  /* Clean empty lines at the end */
  while (xbt_dynar_length(diff) > 0) {
    char *str;
    xbt_dynar_pop(diff, &str);
    if (str[0] == '\0' || !strcmp(str, "  ")) {
      free(str);
    } else {
      xbt_dynar_push(diff, &str);
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
