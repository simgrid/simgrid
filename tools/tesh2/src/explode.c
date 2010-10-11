#include <explode.h>
 char ** explode(char separator, const char *string) 
{
  int pos = 0;
  int i, len = 1;
  int number = 2;
  char **table;
  const char *p = string;
  for (i = 0; p[i] != '\0'; i++)
    if (p[i] == separator)
      number++;
  table = (char **) calloc(number, sizeof(char *));
  i = 0;
  while (*p++ != '\0')
     {
    if (*p == separator)
       {
      table[i] = (char *) calloc(len + 1, sizeof(char));
      strncpy(table[i], string + pos, len);
      pos += len + 1;
      len = 0;
      i++;
      }
    
    else
      len++;
    }
  if (len > 1)
     {
    table[i] = (char *) calloc(len + 1, sizeof(char));
    strncpy(table[i], string + pos, len);
    }
  table[++i] = NULL;
  return table;
}


