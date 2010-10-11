#include <directory.h>
#include <fstreams.h>
#include <fstream.h>
 XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);
directory_t  directory_new(const char *name) 
{
  directory_t directory;
  if (!name)
     {
    errno = EINVAL;
    return NULL;
    }
  directory = xbt_new0(s_directory_t, 1);
  directory->name = strdup(name);
  directory->stream = NULL;
  return directory;
}

int  directory_open(directory_t directory) 
{
  if (!directory || directory->stream)
    return EINVAL;
  if (!(directory->stream = opendir(directory->name)))
    return errno;
  return 0;
}

int  directory_close(directory_t directory) 
{
  if (!directory)
    return EINVAL;
  if (!directory->stream)
    return EBADF;
  if (closedir(directory->stream))
    return errno;
  directory->stream = NULL;
  return 0;
}

int 
directory_load(directory_t directory, fstreams_t fstreams,
               xbt_dynar_t suffixes) 
{
  struct dirent *entry = { 0 };
  s_fstream_t sfstream = {
  0};
  char *suffix;
  unsigned int i;
  int has_valid_suffix;
  int is_empty = 1;
  int rv;
  if (!directory || !fstreams)
    return EINVAL;
  if (!directory->stream)
    return EBADF;
  sfstream.directory = strdup(directory->name);
  while ((entry = readdir(directory->stream)))
     {
    has_valid_suffix = 0;
    xbt_dynar_foreach(suffixes, i, suffix)  {
      if (!strncmp
           (suffix,
            entry->d_name + (strlen(entry->d_name) - strlen(suffix)),
            strlen(suffix)))
         {
        has_valid_suffix = 1;
        break;
        }
    }
    if (!has_valid_suffix)
      continue;
    sfstream.name = strdup(entry->d_name);
    
        /* check first if the file stream is already in the file streams to run */ 
        if (fstreams_contains(fstreams, &sfstream))
       {
      WARN1("file %s already registred", entry->d_name);
      free(sfstream.name);
      continue;
      }
    
        /* add the fstream to the list of file streams to run */ 
        if ((rv =
             fstreams_add(fstreams,
                          fstream_new(directory->name, entry->d_name))))
       {
      free(sfstream.directory);
      free(sfstream.name);
      return rv;
      }
    is_empty = 0;
    free(sfstream.name);
    }
  if (is_empty)
    WARN1("no tesh file found in the directory %s", directory->name);
  free(sfstream.directory);
  return 0;
}

int  directory_free(void **directoryptr) 
{
  directory_t directory;
  if (!(*directoryptr))
    return EINVAL;
  directory = *((directory_t *) directoryptr);
  if (directory->stream)
    if (directory_close(directory))
      return errno;
  if (directory->name)
    free(directory->name);
  free(*directoryptr);
  *directoryptr = NULL;
  return 0;
}

const char * directory_get_name(directory_t directory) 
{
  if (!directory)
     {
    errno = EINVAL;
    return NULL;
    }
  return (const char *) directory->name;
} 
