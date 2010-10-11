#include <str_replace.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <stdio.h>
int 
str_replace(char **str, const char *what, const char *with,
            const char *delimiters) 
{
  size_t pos, i, len;
  char *begin = NULL;
  char *buf;
  int size;
  if (!*str || !what)
     {
    errno = EINVAL;
    return -1;
    }
  if (delimiters)
     {
    char *delimited;
    if (!(delimited = (char *) calloc((strlen(what) + 2), sizeof(char))))
      return -1;
    len = strlen(delimiters);
    for (i = 0; i < len; i++)
       {
      memset(delimited, 0, (strlen(what) + 2));
      sprintf(delimited, "%s%c", what, delimiters[i]);
      if ((begin = strstr(*str, delimited)))
        break;
      }
    free(delimited);
    }
  
  else
    begin = strstr(*str, what);
  if (!begin && (size = (int) strlen(*str) - (int) strlen(what)) >= 0
         && !strcmp(*str + size, what))
    begin = strstr(*str, what);
  if (!begin)
     {
    errno = ESRCH;
    return -1;
    }
  pos = begin - *str;
  i = 0;
  pos += strlen(what);
  if (begin == *str)
     {
    if (!
         (buf =
          (char *) calloc((with ? strlen(with) : 0) +
                          ((pos <
                            strlen(*str)) ? strlen(*str + pos) : 0) + 1,
                          sizeof(char))))
      return -1;
    if (with)
      strcpy(buf, with);
    if (pos < strlen(*str))
      strcpy(buf + (with ? strlen(with) : 0), *str + pos);
    }
  
  else
     {
    if (!
         (buf =
          (char *) calloc((begin - *str) + (with ? strlen(with) : 0) +
                          ((pos <
                            strlen(*str)) ? strlen(*str + pos) : 0) + 1,
                          sizeof(char))))
      return -1;
    strncpy(buf, *str, (begin - *str));
    if (with)
      strcpy(buf + (begin - *str), with);
    if (pos < strlen(*str))
      strcpy(buf + (begin - *str) + (with ? strlen(with) : 0),
              *str + pos);
    }
  free(*str);
  *str = buf;
  return 0;
}

int 
str_replace_all(char **str, const char *what, const char *with,
                const char *delimiters) 
{
  int rv;
  while (!(rv = str_replace(str, what, with, delimiters)));
  return (errno == ESRCH) ? 0 : -1;
}


