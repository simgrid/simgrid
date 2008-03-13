#ifndef __DIRECTORIES_H
#define __DIRECTORIES_H


#include <com.h>

#ifdef __cplusplus
extern "C" {
#endif

directories_t
directories_new(void);

int
directories_add(directories_t directories, directory_t directory);

int
directories_contains(directories_t directories, directory_t directory);

int
directories_load(directories_t directories, fstreams_t fstreams, lstrings_t suffixes);

int
directories_free(void** directoriesptr);

directory_t
directories_get_back(directories_t directories);

directory_t
directories_search_fstream_directory(directories_t directories, const char* name);

int
directories_get_size(directories_t directories);

int
directories_has_directories_to_load(directories_t directories);

#ifdef __cplusplus
}
#endif

#endif /*!__DIRECTORIES_H */
