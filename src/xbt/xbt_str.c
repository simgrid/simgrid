/* xbt_str.c - various helping functions to deal with strings               */

/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
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
 *  - " "    (ASCII 32  (0x20))  space.
 *  - "\t"    (ASCII 9  (0x09))  tab.
 *  - "\n"    (ASCII 10  (0x0A))  line feed.
 *  - "\r"    (ASCII 13  (0x0D))  carriage return.
 *  - "\0"    (ASCII 0  (0x00))  NULL.
 *  - "\x0B"  (ASCII 11  (0x0B))  vertical tab.
 *
 * @param s The string to strip. Modified in place.
 * @param char_list A string which contains the characters you want to strip.
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
 *  - " "    (ASCII 32  (0x20))  space.
 *  - "\t"    (ASCII 9  (0x09))  tab.
 *  - "\n"    (ASCII 10  (0x0A))  line feed.
 *  - "\r"    (ASCII 13  (0x0D))  carriage return.
 *  - "\0"    (ASCII 0  (0x00))  NULL.
 *  - "\x0B"  (ASCII 11  (0x0B))  vertical tab.
 *
 * @param s The string to strip. Modified in place.
 * @param char_list A string which contains the characters you want to strip.
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
 *  - " "    (ASCII 32  (0x20))  space.
 *  - "\t"    (ASCII 9  (0x09))  tab.
 *  - "\n"    (ASCII 10  (0x0A))  line feed.
 *  - "\r"    (ASCII 13  (0x0D))  carriage return.
 *  - "\0"    (ASCII 0  (0x00))  NULL.
 *  - "\x0B"  (ASCII 11  (0x0B))  vertical tab.
 *
 * @param s The string to strip.
 * @param char_list A string which contains the characters you want to strip.
 */
void xbt_str_trim(char *s, const char *char_list)
{
  if (!s)
    return;

  xbt_str_rtrim(s, char_list);
  xbt_str_ltrim(s, char_list);
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
 * You can provide a default value to use if the variable is not set in the dict by using '${var:=default}' or
 * '${var:-default}'. These two forms are equivalent, even if they shouldn't to respect the shell standard (:= form
 * should set the value in the dict, but does not) (BUG).
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
 *  - " "    (ASCII 32  (0x20))  space.
 *  - "\t"    (ASCII 9  (0x09))  tab.
 *  - "\n"    (ASCII 10  (0x0A))  line feed.
 *  - "\r"    (ASCII 13  (0x0D))  carriage return.
 *  - "\0"    (ASCII 0  (0x00))  NULL.
 *  - "\x0B"  (ASCII 11  (0x0B))  vertical tab.
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
      to_push = xbt_malloc(v + 1);
      memcpy(to_push, p, v);
      to_push[v] = '\0';
      xbt_dynar_push(res, &to_push);
      done = 1;
    } else {
      //get the appearance
      to_push = xbt_malloc(q - p + 1);
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
 * So if you don't store that argument elsewhere, you should free it in addition to freeing the dynar. This can be done
 * by simply freeing the first argument of the dynar:
 *  free(xbt_dynar_get_ptr(dynar,0));
 *
 * Actually this function puts a bunch of \0 in the memory area you passed as argument to separate the elements, and
 * pushes the address of each chunk in the resulting dynar. Yes, that's uneven. Yes, that's gory. But that's efficient.
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

  /* do not trim leading spaces: caller responsibility to clean his cruft */
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
        THROWF(arg_error, 0, "End of string found while searching for %c in %s", (in_simple_quote ? '\'' : '"'), s);
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
 * It basically does the same argument separation than the shell, where white spaces can be escaped and where arguments
 * are never split within a quote group.
 * Several subsequent spaces are ignored (unless within quotes, of course).
 * You may want to trim the input string, if you want to avoid empty entries
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

/** @brief creates a new string containing what can be read on a fd */
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

/** @brief Parse an integer out of a string, or raise an error
 *
 * The #str is passed as argument to your #error_msg, as follows:
 *       THROWF(arg_error, 0, error_msg, str);
 */
long int xbt_str_parse_int(const char* str, const char* error_msg)
{
  char *endptr;
  if (str == NULL || str[0] == '\0')
    THROWF(arg_error, 0, error_msg, str);

  long int res = strtol(str, &endptr, 10);
  if (endptr[0] != '\0')
    THROWF(arg_error, 0, error_msg, str);

  return res;
}

/** @brief Parse a double out of a string, or raise an error
 *
 * The #str is passed as argument to your #error_msg, as follows:
 *       THROWF(arg_error, 0, error_msg, str);
 */
double xbt_str_parse_double(const char* str, const char* error_msg)
{
  char *endptr;
  if (str == NULL || str[0] == '\0')
    THROWF(arg_error, 0, error_msg, str);

  double res = strtod(str, &endptr);
  if (endptr[0] != '\0')
    THROWF(arg_error, 0, error_msg, str);

  return res;
}

#ifdef SIMGRID_TEST
#include "xbt/str.h"

XBT_TEST_SUITE("xbt_str", "String Handling");

#define mytest(name, input, expected) \
  xbt_test_add(name); \
  d=xbt_str_split_quoted(input); \
  s=xbt_str_join(d,"XXX"); \
  xbt_test_assert(!strcmp(s,expected),\
                   "Input (%s) leads to (%s) instead of (%s)", \
                   input,s,expected);\
                   free(s); \
                   xbt_dynar_free(&d);
XBT_TEST_UNIT("xbt_str_split_quoted", test_split_quoted, "test the function xbt_str_split_quoted")
{
  xbt_dynar_t d;
  char *s;

  mytest("Empty", "", "");
  mytest("Basic test", "toto tutu", "totoXXXtutu");
  mytest("Useless backslashes", "\\t\\o\\t\\o \\t\\u\\t\\u", "totoXXXtutu");
  mytest("Protected space", "toto\\ tutu", "toto tutu");
  mytest("Several spaces", "toto   tutu", "totoXXXtutu");
  mytest("LTriming", "  toto tatu", "totoXXXtatu");
  mytest("Triming", "  toto   tutu  ", "totoXXXtutu");
  mytest("Single quotes", "'toto tutu' tata", "toto tutuXXXtata");
  mytest("Double quotes", "\"toto tutu\" tata", "toto tutuXXXtata");
  mytest("Mixed quotes", "\"toto' 'tutu\" tata", "toto' 'tutuXXXtata");
  mytest("Backslashed quotes", "\\'toto tutu\\' tata", "'totoXXXtutu'XXXtata");
  mytest("Backslashed quotes + quotes", "'toto \\'tutu' tata", "toto 'tutuXXXtata");
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

#define test_parse_error(function, name, variable, str)                 \
  do {                                                                  \
    xbt_test_add(name);                                                 \
    TRY {                                                               \
      variable = function(str, "Parse error");                          \
      xbt_test_fail("The test '%s' did not detect the problem",name );  \
    } CATCH(e) {                                                        \
      if (e.category != arg_error) {                                    \
        xbt_test_exception(e);                                          \
      } else {                                                          \
        xbt_ex_free(e);                                                 \
      }                                                                 \
    }                                                                   \
  } while (0)
#define test_parse_ok(function, name, variable, str, value)             \
  do {                                                                  \
    xbt_test_add(name);                                                 \
    TRY {                                                               \
      variable = function(str, "Parse error");                          \
    } CATCH(e) {                                                        \
      xbt_test_exception(e);                                            \
    }                                                                   \
    xbt_test_assert(variable == value, "Fail to parse '%s'", str);      \
  } while (0)

XBT_TEST_UNIT("xbt_str_parse", test_parse, "Test the parsing functions")
{
  xbt_ex_t e;
  int rint = -9999;
  test_parse_ok(xbt_str_parse_int, "Parse int", rint, "42", 42);
  test_parse_ok(xbt_str_parse_int, "Parse 0 as an int", rint, "0", 0);
  test_parse_ok(xbt_str_parse_int, "Parse -1 as an int", rint, "-1", -1);

  test_parse_error(xbt_str_parse_int, "Parse int + noise", rint, "342 cruft");
  test_parse_error(xbt_str_parse_int, "Parse NULL as an int", rint, NULL);
  test_parse_error(xbt_str_parse_int, "Parse '' as an int", rint, "");
  test_parse_error(xbt_str_parse_int, "Parse cruft as an int", rint, "cruft");

  double rdouble = -9999;
  test_parse_ok(xbt_str_parse_double, "Parse 42 as a double", rdouble, "42", 42);
  test_parse_ok(xbt_str_parse_double, "Parse 42.5 as a double", rdouble, "42.5", 42.5);
  test_parse_ok(xbt_str_parse_double, "Parse 0 as a double", rdouble, "0", 0);
  test_parse_ok(xbt_str_parse_double, "Parse -1 as a double", rdouble, "-1", -1);

  test_parse_error(xbt_str_parse_double, "Parse double + noise", rdouble, "342 cruft");
  test_parse_error(xbt_str_parse_double, "Parse NULL as a double", rdouble, NULL);
  test_parse_error(xbt_str_parse_double, "Parse '' as a double", rdouble, "");
  test_parse_error(xbt_str_parse_double, "Parse cruft as a double", rdouble, "cruft");
}
#endif                          /* SIMGRID_TEST */
