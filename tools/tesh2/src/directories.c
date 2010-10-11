#include <directories.h>
#include <directory.h>
 XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);
directories_t  directories_new(void) 
{
  directories_t directories = xbt_new0(s_directories_t, 1);
  directories->items =
      xbt_dynar_new(sizeof(directory_t), (void_f_pvoid_t) directory_free);
  return directories;
}

int  directories_get_size(directories_t directories) 
{
  if (!directories)
     {
    errno = EINVAL;
    return -1;
    }
  return xbt_dynar_length(directories->items);
}

int  directories_is_empty(directories_t directories) 
{
  if (!directories)
     {
    errno = EINVAL;
    return -1;
    }
  return (0 == xbt_dynar_length(directories->items));
}

int  directories_add(directories_t directories, directory_t directory) 
{
  directory_t cur;
  unsigned int i;
  if (!directories)
    return EINVAL;
  xbt_dynar_foreach(directories->items, i, cur)  {
    if (!strcmp(cur->name, directory->name))
      return EEXIST;
  }
  xbt_dynar_push(directories->items, &directory);
  return 0;
}

int 
directories_contains(directories_t directories, directory_t directory) 
{
  directory_t * cur;
  unsigned int i;
  if (!directories)
    return EINVAL;
  xbt_dynar_foreach(directories->items, i, cur)  {
    if (!strcmp((*cur)->name, directory->name))
      return 1;
  }
  return 0;
}

int 
directories_load(directories_t directories, fstreams_t fstreams,
                 xbt_dynar_t suffixes) 
{
  directory_t directory;
  int rv;
  unsigned int i;
  if (!directories || !fstreams || !suffixes)
    return EINVAL;
  xbt_dynar_foreach(directories->items, i, directory)  {
    if ((rv = directory_open(directory)))
      return rv;
    if ((rv = directory_load(directory, fstreams, suffixes)))
      return rv;
    if ((rv = directory_close(directory)))
      return rv;
  }
  return 0;
}

int  directories_free(void **directoriesptr) 
{
  directories_t directories;
  if (!(*directoriesptr))
    return EINVAL;
  directories = (directories_t) (*directoriesptr);
  if (directories->items)
    xbt_dynar_free(&(directories->items));
  free(*directoriesptr);
  *directoriesptr = NULL;
  return 0;
}


