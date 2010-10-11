#ifndef __DIRENT_H
#define __DIRENT_H

#include <windows.h>
#include <sys/types.h>
    
#ifndef S_ISDIR
#define	S_ISDIR(__mode)	(((__mode) & S_IFMT) == S_IFDIR)
#endif  /*  */
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
   struct dirent  {
    char d_name[MAX_PATH + 1];
  };
    typedef struct s_DIR  {
    HANDLE file_handle;
    DWORD pos;
    char directory_name[MAX_PATH + 1];
     struct dirent entry;
  } DIR, *DIR_t;
  DIR * opendir(const char *directory_name);
  struct dirent *readdir(DIR * dir);
  void  rewinddir(DIR * dir);
    int  closedir(DIR * dir);
    off_t  telldir(DIR * dir);
    void  seekdir(DIR * dir, off_t offset);
   
#ifdef __cplusplus
extern} 
#endif  /*  */

#endif  /* !__DIRENT_H */
