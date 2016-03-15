/* xbt_os_file.c -- portable interface to file-related functions            */

/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/file.h"    /* this module */
#include "xbt/log.h"
#include "src/internal_config.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include "libgen.h" /* POSIX dirname */

/** @brief Get a single line from the stream (reimplementation of the GNU getline)
 *
 * This is a reimplementation of the GNU getline function, so that our code don't depends on the GNU libc.
 *
 * xbt_getline() reads an entire line from stream, storing the address of the buffer containing the text into *buf.
 * The buffer is null-terminated and includes the newline character, if one was found.
 *
 * If *buf is NULL, then xbt_getline() will allocate a buffer for storing the  line, which should be freed by the user
 * program.
 *
 * Alternatively, before calling xbt_getline(), *buf can contain a pointer to a malloc()-allocated buffer *n bytes in
 * size. If the buffer is not large enough to hold the line, xbt_getline() resizes it with realloc(), updating
 * *buf and *n as necessary.
 *
 * In either case, on a successful call, *buf and *n will be updated to reflect the buffer address and allocated size
 * respectively.
 */
ssize_t xbt_getline(char **buf, size_t *n, FILE *stream)
{
  ssize_t i;
  int ch;

  ch = getc(stream);
  if (ferror(stream) || feof(stream))
    return -1;

  if (!*buf) {
    *n = 512;
    *buf = xbt_malloc(*n);
  }

  i = 0;
  do {
    if (i == *n)
      *buf = xbt_realloc(*buf, *n += 512);
    (*buf)[i++] = ch;
  } while (ch != '\n' && (ch = getc(stream)) != EOF);

  if (i == *n)
    *buf = xbt_realloc(*buf, *n += 1);
  (*buf)[i] = '\0';

  return i;
}

/** @brief Returns the directory component of a path (reimplementation of POSIX dirname)
 *
 * The argument is never modified, and the returned value must be freed after use.
 */
char *xbt_dirname(const char *path) {
  char *tmp = xbt_strdup(path);
  char *res = xbt_strdup(dirname(tmp));
  free(tmp);
  return res;
}

/** @brief Returns the file component of a path (reimplementation of POSIX basename)
 *
 * The argument is never modified, and the returned value must be freed after use.
 */
char *xbt_basename(const char *path) {
  char *tmp = xbt_strdup(path);
  char *res = xbt_strdup(basename(tmp));
  free(tmp);
  return res;
}
