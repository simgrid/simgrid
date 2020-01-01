/* xbt_str.cpp - various helping functions to deal with strings             */

/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "xbt/misc.h"
#include "xbt/str.h" /* headers of these functions */
#include "xbt/string.hpp"

/** @brief Splits a string into a dynar of strings
 *
 * @param s: the string to split
 * @param sep: a string of all chars to consider as separator.
 *
 * By default (with sep=nullptr), these characters are used as separator:
 *
 *  - " "    (ASCII 32  (0x20))  space.
 *  - "\t"    (ASCII 9  (0x09))  tab.
 *  - "\n"    (ASCII 10  (0x0A))  line feed.
 *  - "\r"    (ASCII 13  (0x0D))  carriage return.
 *  - "\0"    (ASCII 0  (0x00))  nullptr.
 *  - "\x0B"  (ASCII 11  (0x0B))  vertical tab.
 */
xbt_dynar_t xbt_str_split(const char *s, const char *sep)
{
  xbt_dynar_t res = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  const char *sep_dflt = " \t\n\r\x0B";
  char is_sep[256] = { 1, 0 };

  /* check what are the separators */
  memset(is_sep, 0, sizeof(is_sep));
  if (not sep) {
    while (*sep_dflt)
      is_sep[(unsigned char) *sep_dflt++] = 1;
  } else {
    while (*sep)
      is_sep[(unsigned char) *sep++] = 1;
  }
  is_sep[0] = 1; /* End of string is also separator */

  /* Do the job */
  const char* p = s;
  const char* q = s;
  int done      = 0;

  if (s[0] == '\0')
    return res;

  while (not done) {
    char *topush;
    while (not is_sep[(unsigned char)*q]) {
      q++;
    }
    if (*q == '\0')
      done = 1;

    topush = (char*) xbt_malloc(q - p + 1);
    memcpy(topush, p, q - p);
    topush[q - p] = '\0';
    xbt_dynar_push(res, &topush);
    p = ++q;
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
  xbt_dynar_t res = xbt_dynar_new(sizeof(char *), nullptr);
  char* beg;
  char* end; /* pointers around the parsed chunk */
  int in_simple_quote = 0;
  int in_double_quote = 0;
  int done            = 0;
  int ctn             = 0; /* Got something in this block */

  if (s[0] == '\0')
    return res;

  beg = s;

  /* do not trim leading spaces: caller responsibility to clean his cruft */
  end = beg;

  while (not done) {
    switch (*end) {
    case '\\':
      ctn = 1;
      /* Protected char; move it closer */
      memmove(end, end + 1, strlen(end));
      if (*end == '\0')
        throw std::invalid_argument("String ends with \\");
      end++;                    /* Pass the protected char */
      break;
    case '\'':
      ctn = 1;
      if (not in_double_quote) {
        in_simple_quote = not in_simple_quote;
        memmove(end, end + 1, strlen(end));
      } else {
        /* simple quote protected by double ones */
        end++;
      }
      break;
    case '"':
      ctn = 1;
      if (not in_simple_quote) {
        in_double_quote = not in_double_quote;
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
        throw std::invalid_argument(simgrid::xbt::string_printf("End of string found while searching for %c in %s",
                                                                (in_simple_quote ? '\'' : '"'), s));
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
  xbt_free(str_to_free);
  xbt_dynar_shrink(res, 0);
  xbt_dynar_free(&parsed);
  return res;
}

/** @brief Parse an integer out of a string, or raise an error
 *
 * The @a str is passed as argument to your @a error_msg, as follows:
 * @verbatim throw std::invalid_argument(simgrid::xbt::string_printf(error_msg, str)); @endverbatim
 */
long int xbt_str_parse_int(const char* str, const char* error_msg)
{
  char* endptr;
  if (str == nullptr || str[0] == '\0')
    throw std::invalid_argument(simgrid::xbt::string_printf(error_msg, str));

  long int res = strtol(str, &endptr, 10);
  if (endptr[0] != '\0')
    throw std::invalid_argument(simgrid::xbt::string_printf(error_msg, str));

  return res;
}

/** @brief Parse a double out of a string, or raise an error
 *
 * The @a str is passed as argument to your @a error_msg, as follows:
 * @verbatim throw std::invalid_argument(simgrid::xbt::string_printf(error_msg, str)); @endverbatim
 */
double xbt_str_parse_double(const char* str, const char* error_msg)
{
  char *endptr;
  if (str == nullptr || str[0] == '\0')
    throw std::invalid_argument(simgrid::xbt::string_printf(error_msg, str));

  double res = strtod(str, &endptr);
  if (endptr[0] != '\0')
    throw std::invalid_argument(simgrid::xbt::string_printf(error_msg, str));

  return res;
}
