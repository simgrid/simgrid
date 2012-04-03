/*
 * file_stat.h
 *
 *  Created on: 3 avr. 2012
 *      Author: navarro
 */

#ifndef _FILE_STAT_H
#define _FILE_STAT_H

#include "xbt/sysdep.h"

typedef struct file_stat {
  char *user_rights;
  char *user;
  char *group;
  char *date; /* FIXME: update to time_t or double */
  char *time; /* FIXME: update to time_t or double */
  size_t size;
} s_file_stat_t, *file_stat_t;

static XBT_INLINE void file_stat_copy(s_file_stat_t *src, s_file_stat_t *dst)
{
    dst->date = xbt_strdup(src->date);
    dst->group = xbt_strdup(src->group);
    dst->size = src->size;
    dst->time = xbt_strdup(src->time);
    dst->user = xbt_strdup(src->user);
    dst->user_rights = xbt_strdup(src->user_rights);
}

#endif /* _FILE_STAT_H */
