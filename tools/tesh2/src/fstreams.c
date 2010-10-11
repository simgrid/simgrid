#include <fstreams.h>
#include <excludes.h>
#include <fstream.h>
 XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);
fstreams_t  fstreams_new(void_f_pvoid_t fn_finalize) 
{
  fstreams_t fstreams = xbt_new0(s_fstreams_t, 1);
  fstreams->items = xbt_dynar_new(sizeof(fstream_t), fn_finalize);
  return fstreams;
}

int  fstreams_exclude(fstreams_t fstreams, excludes_t excludes) 
{
  fstream_t fstream;
  unsigned int i;
  if (!fstreams || !excludes)
    return EINVAL;
  if (excludes_is_empty(excludes))
    return 0;
  
      /* collecte the file streams to exclude */ 
      xbt_dynar_foreach(fstreams->items, i, fstream)  {
    if (excludes_contains(excludes, fstream))
       {
      INFO1("excluding %s", fstream->name);
      xbt_dynar_cursor_rm(fstreams->items, &i);
      }
  }
  return 0;
}

int  fstreams_contains(fstreams_t fstreams, fstream_t fstream) 
{
  fstream_t cur;
  unsigned int i;
  if (!fstreams || !fstream)
     {
    errno = EINVAL;
    return 0;
    }
  xbt_dynar_foreach(fstreams->items, i, cur)  {
    if (!strcmp(cur->name, fstream->name)
         && !strcmp(cur->directory, fstream->directory))
      return 1;
  }
  return 0;
}

int  fstreams_load(fstreams_t fstreams) 
{
  fstream_t fstream;
  unsigned int i;
  if (!fstreams)
    return EINVAL;
  xbt_dynar_foreach(fstreams->items, i, fstream)  {
    fstream_open(fstream);
  }
  return 0;
}

int  fstreams_add(fstreams_t fstreams, fstream_t fstream) 
{
  if (!fstreams)
    return EINVAL;
  xbt_dynar_push(fstreams->items, &fstream);
  return 0;
}

int  fstreams_free(void **fstreamsptr) 
{
  if (!(*fstreamsptr))
    return EINVAL;
  if ((*((fstreams_t *) fstreamsptr))->items)
    xbt_dynar_free(&((*((fstreams_t *) fstreamsptr))->items));
  free(*fstreamsptr);
  *fstreamsptr = NULL;
  return 0;
}

int  fstreams_get_size(fstreams_t fstreams) 
{
  if (!fstreams)
     {
    errno = EINVAL;
    return -1;
    }
  return xbt_dynar_length(fstreams->items);
}

int  fstreams_is_empty(fstreams_t fstreams) 
{
  if (!fstreams)
     {
    errno = EINVAL;
    return -1;
    }
  return (0 == xbt_dynar_length(fstreams->items));
}


