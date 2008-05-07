#ifndef __dictionary_H
#define __dictionary_H

#include <htable.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __FN_FINALIZE_T_DEFINED
typedef int (*fn_finalize_t)(void**);
#define __FN_FINALIZE_T_DEFINED
#endif

typedef struct s_dictionary
{
	htable_t htable;
}s_dictionary_t,* dictionary_t;


dictionary_t
dictionary_new(fn_finalize_t fn_finalize);

int
dictionary_set(dictionary_t dictionary,const char* key, const void* val);

void*
dictionary_get(dictionary_t dictionary,const char* key);

int
dictionary_free(dictionary_t* dictionaryptr);

int
dictionary_clear(dictionary_t dictionary);

int
dictionary_get_size(dictionary_t dictionary);

int
dictionary_is_empty(dictionary_t dictionary);

void*
dictionary_remove(dictionary_t dictionary,const char* key);

#ifdef __cplusplus
}
#endif


#endif /* !__dictionary_H */
