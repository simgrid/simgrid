#include <excludes.h>
#include <fstream.h>
 XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);
excludes_t  excludes_new(void) 
{
  excludes_t excludes = xbt_new0(s_excludes_t, 1);
  excludes->items =
      xbt_dynar_new(sizeof(fstream_t), (void_f_pvoid_t) fstream_free);
  return excludes;
}

int  excludes_is_empty(excludes_t excludes) 
{
  if (!excludes)
     {
    errno = EINVAL;
    return 0;
    }
  return (0 == xbt_dynar_length(excludes->items));
}

int  excludes_contains(excludes_t excludes, fstream_t fstream) 
{
  fstream_t cur;
  unsigned int i;
  if (!excludes || !fstream)
     {
    errno = EINVAL;
    return 0;
    }
  xbt_dynar_foreach(excludes->items, i, cur)  {
    if (!strcmp(fstream->name, cur->name)
         && !strcmp(fstream->directory, cur->directory))
      return 1;
  }
  return 0;
}

int  excludes_add(excludes_t excludes, fstream_t fstream) 
{
  if (!excludes)
    return EINVAL;
  if (excludes_contains(excludes, fstream))
    return EEXIST;
  xbt_dynar_push(excludes->items, &fstream);
  return 0;
}

int  excludes_check(excludes_t excludes, fstreams_t fstreams) 
{
  fstream_t exclude;
  fstream_t fstream;
  int success = 1;
  int exists;
  unsigned int i;
  if (!excludes || !fstreams)
    return EINVAL;
  xbt_dynar_foreach(excludes->items, i, exclude)  {
    exists = 1;
    xbt_dynar_foreach(fstreams->items, i, fstream)  {
      exists = 0;
      if (!strcmp(fstream->name, exclude->name)
            && !strcmp(fstream->directory, exclude->directory))
         {
        exists = 1;
        break;
        }
    }
    if (!exists)
       {
      success = 0;
      WARN1("cannot exclude the file %s", exclude->name);
      }
  }
  return success;
}

int  excludes_free(void **excludesptr) 
{
  if (!(*excludesptr))
    return EINVAL;
  if ((*((excludes_t *) excludesptr))->items)
    xbt_dynar_free((&(*((excludes_t *) excludesptr))->items));
  free(*excludesptr);
  *excludesptr = NULL;
  return 0;
}


