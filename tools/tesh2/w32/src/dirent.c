
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
    DIR * opendir(const char *directory_name) 
{
  struct stat sb;
  DIR * dir;
  if (NULL == directory_name)
     {
    errno = EINVAL;
    return NULL;
    }
  if (0 != stat(directory_name, &sb))
     {
    
        /* directory not found */ 
        errno = ENOENT;
    return NULL;
    }
  if (0 == S_ISDIR(sb.st_mode))
     {
    
        /* it's not a directory */ 
        errno = ENOTDIR;
    return NULL;
    }
  dir = (DIR *) calloc(1, sizeof(DIR));
  if (NULL == dir)
     {
    errno = ENOMEM;
    return NULL;
    }
  if ('\\' != dir->directory_name[strlen(directory_name) - 1])
    sprintf(dir->directory_name, "%s\\*", directory_name);
  
  else
    sprintf(dir->directory_name, "%s*", directory_name);
  dir->file_handle = INVALID_HANDLE_VALUE;
  return dir;
}

int  closedir(DIR * dir) 
{
  if (NULL == dir)
    return EINVAL;
  if (INVALID_HANDLE_VALUE != dir->file_handle)
    FindClose(dir->file_handle);
  
  else
    return EBADF;
  free(dir);
  return 0;
}

struct dirent *readdir(DIR * dir) 
{
  WIN32_FIND_DATA wfd = {
  0};
  if (!dir)
     {
    errno = EINVAL;
    return NULL;
    }
  if (!dir->pos)
    dir->file_handle = FindFirstFile(dir->directory_name, &wfd);
  if (!FindNextFile(dir->file_handle, &wfd))
    return NULL;
  dir->pos++;
  strcpy(dir->entry.d_name, wfd.cFileName);
  return &(dir->entry);
}

void  rewinddir(DIR * dir) 
{
  if (NULL == dir)
     {
    errno = EINVAL;
    return;
    }
  if (INVALID_HANDLE_VALUE != dir->file_handle)
     {
    FindClose(dir->file_handle);
    dir->file_handle = INVALID_HANDLE_VALUE;
    dir->pos = 0;
    }
}

off_t  telldir(DIR * dir) 
{
  if (NULL == dir)
     {
    errno = EINVAL;
    return -1;
    }
  return dir->pos;
}

void  seekdir(DIR * dir, off_t offset) 
{
  WIN32_FIND_DATA wfd = {
  0};
  if (NULL == dir)
     {
    errno = EINVAL;
    return;
    }
  if (INVALID_HANDLE_VALUE != dir->file_handle)
     {
    FindClose(dir->file_handle);
    dir->file_handle = INVALID_HANDLE_VALUE;
    dir->pos = 0;
    }
  dir->file_handle = FindFirstFile(dir->directory_name, &wfd);
  dir->pos += offset;
  while (--offset)
     {
    if (!FindNextFile(dir->file_handle, &wfd))
      errno = EINVAL;
    }
}


